# /// script
# requires-python = ">=3.11,<3.14"
# dependencies = [
#     "ifcopenshell>=0.8",
#     "trimesh>=4.4",
#     "manifold3d>=3.0",
#     "numpy",
#     "shapely",
#     "mapbox-earcut",
#     "networkx",
#     "scipy",
# ]
# ///

# /// tapir
# category = "import-export"
# archicad-versions = [28]
# min-tapir-version = "1.6.0"
# changes = "files"
# ///
"""
Stories to STL for 3D printing

Original by Constantin Koblmiller. Licence: MIT.

Exports the elements visible (isolated) in the 3D window as one STL per story,
for 3D printing. The cuts lie at story level + offset (recommended: -0.30 m); the
cut faces are closed so that every part is printable.

How it works (no UI automation):
  1. Tapir: get the elements with the filter "IsVisibleIn3D" (or the selection)
  2. Tapir: select them and save them as IFC ("SelectedElementsOnly") to a
     temporary file -> the previous selection is restored afterwards
  3. IfcOpenShell: triangulate the geometry, story levels from the IfcBuildingStoreys
  4. manifold3d / trimesh: cut per story, close the cut faces, optionally union
     into one body. Basement stories belong to the ground floor; optionally
     everything below the ground-floor cut is exported as a separate base plate.
  5. Write one STL per story at print scale (mm)

Needs the Tapir Add-On 1.6.0 or later.

Testing without Archicad:
  uv run --script stories_to_stl.py --ifc model.ifc --out stl_out --scale 500
"""

from __future__ import annotations

import argparse
import glob
import json
import math
import multiprocessing
import os
import re
import sys
import tempfile
import threading
import time
import traceback
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path

import numpy as np

SCRIPT_TITLE = "Stories to STL for 3D printing"
MIN_TAPIR_VERSION = (1, 6, 0)

# IFC classes that are never printed
EXCLUDED_IFC_CLASSES = ("IfcOpeningElement", "IfcSpace", "IfcAnnotation", "IfcGrid",
                        "IfcVirtualElement", "IfcSite")


def default_output_folder(project: str) -> Path:
    """ Documents/Tapir/STL/<project>_STL """
    return Path.home() / "Documents" / "Tapir" / "STL" / f"{safe_filename(project) or 'Archicad'}_STL"


# ---------------------------------------------------------------------------
#  Archicad / Tapir
# ---------------------------------------------------------------------------

class ArchicadError(Exception):
    pass


class Archicad:
    """ Talks only to Archicad on this computer (127.0.0.1, at the port the palette passes in). """

    def __init__(self, port: int):
        self.url = f"http://127.0.0.1:{port}"

    def command(self, command: str, parameters: dict | None = None) -> dict:
        data = json.dumps({"command": command, "parameters": parameters or {}}).encode("utf8")
        req = urllib.request.Request(self.url, data, {"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(req, timeout=3600) as resp:
                answer = json.loads(resp.read())
        except (urllib.error.URLError, ConnectionError, OSError) as e:
            raise ArchicadError(f"Archicad cannot be reached ({self.url}): {e}") from e
        if not answer.get("succeeded"):
            err = answer.get("error", {})
            raise ArchicadError(f"{command}: {err.get('message', err)}")
        return answer.get("result", {})

    def tapir(self, command: str, parameters: dict | None = None) -> dict:
        result = self.command("API.ExecuteAddOnCommand", {
            "addOnCommandId": {"commandNamespace": "TapirCommand", "commandName": command},
            "addOnCommandParameters": parameters or {},
        })
        response = result.get("addOnCommandResponse", {})
        if isinstance(response, dict) and "error" in response:
            err = response["error"]
            raise ArchicadError(f"{command}: {err.get('message', err)}")
        if isinstance(response, dict) and response.get("success") is False:
            err = response.get("error", {})
            raise ArchicadError(f"{command} failed: {err.get('message', err)}")
        return response

    # --- helpers ----------------------------------------------------------
    def tapir_version(self) -> tuple[int, ...]:
        v = self.tapir("GetAddOnVersion").get("version", "0")
        return tuple(int(x) for x in re.findall(r"\d+", v)[:3])

    def current_window_type(self) -> str:
        return self.tapir("GetCurrentWindowType").get("currentWindowType", "")

    def translators(self) -> list[str]:
        items = self.tapir("GetIFCExportTranslators").get("translators", [])
        return [t["name"] for t in items]

    def project_name(self) -> str:
        """ Name of the Teamwork project or the PLN (without extension). """
        try:
            info = self.tapir("GetProjectInfo")
        except ArchicadError:
            return ""
        if info.get("isUntitled"):
            return "Untitled"
        name = info.get("projectName") or ""
        return re.sub(r"\.(pln|pla|tpl)$", "", name, flags=re.I)

    def stories(self) -> list[tuple[str, float, int]]:
        """ (name, level in m, index) of all stories, bottom to top (index 0 = ground floor). """
        data = self.tapir("GetStories")
        items = sorted(data.get("stories", []), key=lambda st: st.get("level", 0.0))
        return [(st.get("name") or f"Story {st.get('index', '')}", float(st.get("level", 0.0)),
                 int(st.get("index", 0))) for st in items]

    def z_range(self, elements: list[dict]) -> tuple[float, float] | None:
        """ Height range (m) of the elements, from their 3D bounding boxes. """
        zmin, zmax = math.inf, -math.inf
        for i in range(0, len(elements), 2000):
            boxes = self.tapir("Get3DBoundingBoxes", {"elements": elements[i:i + 2000]})
            for b in boxes.get("boundingBoxes3D", []):
                bb = b.get("boundingBox3D") if isinstance(b, dict) else None
                if bb:
                    zmin, zmax = min(zmin, bb["zMin"]), max(zmax, bb["zMax"])
        return (zmin, zmax) if zmin < zmax else None

    def selected(self) -> list[dict]:
        return self.tapir("GetSelectedElements").get("elements", [])

    def visible_in_3d(self) -> list[dict]:
        return self.tapir("GetAllElements", {"filters": ["IsVisibleIn3D"]}).get("elements", [])

    def set_selection(self, elements: list[dict]) -> None:
        current = self.selected()
        wanted = {e["elementId"]["guid"] for e in elements}
        self.tapir("ChangeSelectionOfElements", {
            "removeElementsFromSelection": [e for e in current if e["elementId"]["guid"] not in wanted],
            "addElementsToSelection": elements,
        })

    def save_ifc(self, path: str, translator: str) -> None:
        self.tapir("IFCFileOperation", {
            "method": "save",
            "ifcFilePath": path,
            "fileType": "ifc",
            "translatorName": translator,
            "elementsToExport": "SelectedElementsOnly",
        })

    def alert(self, kind: str, message: str, sub: str = "") -> None:
        try:
            params = {"alertType": kind, "title": SCRIPT_TITLE, "message": message, "button1": "OK"}
            if sub:
                params["subMessage"] = sub
            self.tapir("ShowAlert", params)
        except ArchicadError:
            pass


def export_ifc_from_archicad(ac: Archicad, source: str, translator: str, ifc_path: str, log) -> int:
    """ Exports the chosen elements as IFC. Returns the number of elements. """
    original_selection = ac.selected()
    if source == "selection":
        elements = original_selection
    else:
        log("Looking for the elements visible in the 3D window …")
        elements = ac.visible_in_3d()
    if not elements:
        raise ArchicadError("No elements found. Is anything visible in the 3D window, or selected?")

    log(f"{len(elements)} elements – saving a temporary IFC file …")
    try:
        if source != "selection":
            ac.set_selection(elements)
        ac.save_ifc(ifc_path, translator)
    finally:
        if source != "selection":
            try:
                ac.set_selection(original_selection)
            except ArchicadError:
                pass
    if not os.path.isfile(ifc_path) or os.path.getsize(ifc_path) == 0:
        raise ArchicadError("Archicad did not write an IFC file.")
    return len(elements)


# ---------------------------------------------------------------------------
#  Geometry
# ---------------------------------------------------------------------------

@dataclass
class Storey:
    name: str
    z: float                       # level in m (world coordinates of the IFC)
    offset: float | None = None    # own cut offset (None = general offset)
    cut: bool = True               # False = no cut at its bottom -> joined with the story below
    index: int | None = None       # Archicad story index (0 = ground floor, < 0 = basement)


def load_ifc(ifc_path: str, log, progress=None):
    """ Returns (list of trimesh meshes in m, sorted stories). """
    import ifcopenshell
    import ifcopenshell.geom
    import ifcopenshell.util.placement
    import ifcopenshell.util.unit
    import trimesh

    f = ifcopenshell.open(ifc_path)
    unit_scale = ifcopenshell.util.unit.calculate_unit_scale(f)

    storeys: list[Storey] = []
    for st in f.by_type("IfcBuildingStorey"):
        if st.ObjectPlacement is not None:
            z = float(ifcopenshell.util.placement.get_local_placement(st.ObjectPlacement)[2][3]) * unit_scale
        else:
            z = float(st.Elevation or 0.0) * unit_scale
        storeys.append(Storey(st.Name or "Story", z))
    storeys.sort(key=lambda s: s.z)

    settings = ifcopenshell.geom.settings()
    settings.set("use-world-coords", True)
    try:
        settings.set("weld-vertices", True)
    except Exception:
        pass

    exclude = []
    for cls in EXCLUDED_IFC_CLASSES:
        try:
            exclude += f.by_type(cls)
        except RuntimeError:
            pass
    products = [p for p in f.by_type("IfcProduct") if p.Representation is not None]
    total = max(1, len(products) - len(exclude))

    it = ifcopenshell.geom.iterator(settings, f, max(1, multiprocessing.cpu_count() - 1),
                                    exclude=exclude or None)
    meshes = []
    if it.initialize():
        n = 0
        while True:
            shape = it.get()
            g = shape.geometry
            verts = np.asarray(g.verts, dtype=np.float64).reshape(-1, 3)
            faces = np.asarray(g.faces, dtype=np.int64).reshape(-1, 3)
            if len(faces):
                m = trimesh.Trimesh(verts, faces, process=True)
                if len(m.faces):
                    meshes.append(m)
            n += 1
            if progress and n % 25 == 0:
                progress(min(1.0, n / total))
            if not it.next():
                break
    log(f"{len(meshes)} bodies read from the IFC, {len(storeys)} stories.")
    return meshes, storeys


def assign_story_indices(ifc_storeys: list[Storey], ac_stories: list[tuple[str, float, int]] | None,
                         log) -> None:
    """ Gives the IFC stories their Archicad story index (0 = ground floor). Archicad exports
    all stories, so normally 1:1 by order, else by name + level.
    Without Archicad (command line) the story closest to ±0.00 is the ground floor. """
    if ac_stories and len(ifc_storeys) == len(ac_stories):
        for st, (_, _, idx) in zip(ifc_storeys, ac_stories):
            st.index = idx
        return
    if ac_stories:
        log("Note: the stories of the IFC differ – matching them by name.")
        used = set()
        for name, level, idx in ac_stories:
            cand = [x for x in ifc_storeys if x.name == name and id(x) not in used]
            if cand:
                best = min(cand, key=lambda x: abs(x.z - level))
                used.add(id(best))
                best.index = idx
        if any(x.index is not None for x in ifc_storeys):
            return
    if ifc_storeys:
        eg = min(range(len(ifc_storeys)), key=lambda i: abs(ifc_storeys[i].z))
        for i, st in enumerate(ifc_storeys):
            st.index = i - eg


def configure_storeys(storeys: list[Storey], base_plate: bool, base_offset: float,
                      overrides: dict[int, tuple[bool, float]] | None = None) -> None:
    """ Cut settings per story:
    - basement stories (index < 0): never cut, they belong to the ground floor or the base plate
    - ground floor (index 0): cut only when the base plate is exported separately
    - above: general offset or the individual values (overrides: index -> (cut, offset)) """
    overrides = overrides or {}
    for st in storeys:
        if st.index is None:
            continue
        if st.index < 0:
            st.cut, st.offset = False, None
        elif st.index == 0:
            st.cut, st.offset = base_plate, base_offset
        elif st.index in overrides:
            st.cut, st.offset = overrides[st.index]
        else:
            st.cut, st.offset = True, None


def make_bands(storeys: list[Storey], offset: float, log=None, with_levels: bool = False,
               bottom: float | None = None):
    """ Parts (name, bottom, top) from the stories and their cut settings.
    bottom: bottom of the lowest part relative to the ground floor [m] (None = bottom of
    the model); nothing below it is exported. """
    log = log or (lambda *_: None)
    if not storeys:
        return [("Model", -math.inf, math.inf)]
    zero = [x for x, nxt in zip(storeys, storeys[1:]) if abs(nxt.z - x.z) < 1e-3 and x.index != 0]
    if zero:
        log(f"{len(zero)} story(ies) with a height of 0 skipped.")
        storeys = [x for x in storeys if all(x is not z for z in zero)]
    floor = -math.inf
    if bottom is not None:
        eg = next((x for x in storeys if x.index == 0), storeys[0])
        floor = eg.z + bottom
    groups: list[tuple[list[Storey], float]] = [([], floor)]
    for st in storeys:
        # the ground floor always starts a part of its own (the base plate is below it)
        new_group = st.cut and (groups[-1][0] or st.index == 0)
        if not new_group:
            groups[-1][0].append(st)
            continue
        z0 = st.z + (offset if st.offset is None else st.offset)
        if z0 < groups[-1][1]:
            log(f"Note: the cut at {st.name} is below the previous cut – raised.")
            z0 = groups[-1][1]
        groups.append(([st], z0))
    bands = []
    for k, (members, z0) in enumerate(groups):
        z1 = groups[k + 1][1] if k + 1 < len(groups) else math.inf
        if z1 <= z0:
            continue
        upper = [m for m in members if m.index is None or m.index >= 0]
        if upper:
            name = "+".join(m.name for m in upper)
        elif k + 1 < len(groups):
            name = "Base plate"
        else:
            name = "+".join(m.name for m in members) or "Model"
        level = members[0].z if (members and math.isfinite(z0) and k > 0) else None
        bands.append((name, z0, z1, level) if with_levels else (name, z0, z1))
    return bands


def _to_manifold(mesh):
    import manifold3d as mf
    m = mf.Manifold(mf.Mesh(vert_properties=np.ascontiguousarray(mesh.vertices, dtype=np.float32),
                            tri_verts=np.ascontiguousarray(mesh.faces, dtype=np.uint32)))
    if m.status() != mf.Error.NoError or m.is_empty():
        return None
    return m


def _from_manifold(m):
    import trimesh
    out = m.to_mesh()
    return trimesh.Trimesh(np.asarray(out.vert_properties)[:, :3], np.asarray(out.tri_verts), process=False)


def _slice_trimesh(mesh, z0, z1):
    """ Fallback for bodies that are not closed: planar cuts with a cap. """
    m = mesh
    try:
        if math.isfinite(z0):
            m = m.slice_plane([0, 0, z0], [0, 0, 1], cap=True)
        if m is not None and len(m.faces) and math.isfinite(z1):
            m = m.slice_plane([0, 0, z1], [0, 0, -1], cap=True)
    except Exception:
        # the cap could not be triangulated -> cut open
        m = mesh
        if math.isfinite(z0):
            m = m.slice_plane([0, 0, z0], [0, 0, 1])
        if m is not None and len(m.faces) and math.isfinite(z1):
            m = m.slice_plane([0, 0, z1], [0, 0, -1])
    return m if m is not None and len(m.faces) else None


def _repaired(mesh):
    import trimesh
    m = mesh.copy()
    m.merge_vertices(digits_vertex=5)
    m.update_faces(m.nondegenerate_faces())
    m.update_faces(m.unique_faces())
    m.remove_unreferenced_vertices()
    trimesh.repair.fix_normals(m)
    trimesh.repair.fill_holes(m)
    return m


def _solids(mesh):
    """ Splits an element body into (manifold | None, trimesh) parts.
    Closed parts go through manifold3d (exact cuts, clean unions),
    open ones (e.g. glass panes without thickness) through the trimesh fallback. """
    import warnings
    m = _to_manifold(mesh)
    if m is not None:
        return [(m, mesh)]
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        r = _repaired(mesh)
        m = _to_manifold(r)
        if m is not None:
            return [(m, r)]
        out = []
        for c in r.split(only_watertight=False):
            import trimesh
            trimesh.repair.fix_normals(c)
            out.append((_to_manifold(c), c))
        return out


def _clean_piece(tm, lz0: float, lz1: float, tol: float):
    """ Removes near-zero heights at the cut planes of an element piece:
    - whole piece flatter than tol and at a cut plane -> drop it (None)
    - tops/bottoms closer than tol to the cut plane -> snap them to the plane,
      so that no paper-thin steps against the neighbouring elements remain.
    Returns (manifold | None, trimesh), or None to drop it. """
    import trimesh
    zmin, zmax = tm.bounds[0][2], tm.bounds[1][2]
    near0 = math.isfinite(lz0) and zmin - lz0 < tol
    near1 = math.isfinite(lz1) and lz1 - zmax < tol
    if (near0 or near1) and zmax - zmin < tol:
        return None
    v = tm.vertices.copy()
    z = v[:, 2]
    moved = np.zeros(len(v), bool)
    if math.isfinite(lz1):
        sel = (z > lz1 - tol) & (z < lz1 - 1e-9)
        z[sel], moved[sel] = lz1, True
    if math.isfinite(lz0):
        sel = (z < lz0 + tol) & (z > lz0 + 1e-9)
        z[sel], moved[sel] = lz0, True
    if not moved.any():
        return _to_manifold(tm), tm
    snapped = trimesh.Trimesh(v, tm.faces, process=False)
    snapped.update_faces(snapped.nondegenerate_faces())
    snapped.remove_unreferenced_vertices()
    m = _to_manifold(snapped) if len(snapped.faces) else None
    if m is None:                      # snapping would have broken the body -> original
        return _to_manifold(tm), tm
    return m, snapped


def split_into_bands(meshes, storeys: list[Storey], offset: float, union: bool, log, progress=None,
                     drop_open: bool = True, min_height: float = 0.0, bottom: float | None = None):
    """ Cuts all bodies into story bands [z_i+offset, z_{i+1}+offset).
    min_height (m, model size): flatter slivers are dropped. """
    import manifold3d as mf
    import trimesh

    bands = [(Storey(name, z0), z0, z1) for name, z0, z1 in make_bands(storeys, offset, log, bottom=bottom)]

    # common origin, so that float32 (manifold) stays precise
    origin = np.min([m.bounds[0] for m in meshes], axis=0)
    span = np.max([m.bounds[1] for m in meshes], axis=0) - origin
    big = float(np.max(span)) * 2 + 10.0

    pieces = {id(s): [] for s, _, _ in bands}           # trimesh pieces
    manifold_pieces = {id(s): [] for s, _, _ in bands}  # manifold pieces
    open_parts = 0
    slivers = 0

    for k, mesh in enumerate(meshes):
        local = mesh.copy()
        local.apply_translation(-origin)
        for mani, tm in _solids(local):
            if mani is None:
                open_parts += 1
                if drop_open:
                    continue
            zmin, zmax = tm.bounds[0][2], tm.bounds[1][2]
            for s, z0, z1 in bands:
                lz0, lz1 = z0 - origin[2], z1 - origin[2]
                if zmax <= lz0 + 1e-6 or zmin >= lz1 - 1e-6:
                    continue
                inside = zmin >= lz0 - 1e-6 and zmax <= lz1 + 1e-6
                if mani is None:
                    cut = tm if inside else _slice_trimesh(tm, lz0, lz1)
                    if cut is not None:
                        pieces[id(s)].append(cut)
                    continue
                if inside:
                    cut, cut_tm = mani, tm
                else:
                    bz0, bz1 = max(lz0, zmin - 1.0), min(lz1, zmax + 1.0)
                    box = mf.Manifold.cube([big, big, bz1 - bz0]).translate([-big / 4, -big / 4, bz0])
                    cut = mani ^ box
                    if cut.is_empty():
                        continue
                    cut_tm = _from_manifold(cut)
                if min_height > 0:
                    cleaned = _clean_piece(cut_tm, lz0, lz1, min_height)
                    if cleaned is None:
                        slivers += 1
                        continue
                    if cleaned[0] is not None:
                        cut, cut_tm = cleaned
                if union:
                    manifold_pieces[id(s)].append(cut)
                else:
                    pieces[id(s)].append(cut_tm)
        if progress and k % 20 == 0:
            progress(k / max(1, len(meshes)))

    if open_parts:
        log(f"Note: {open_parts} open part bodies (e.g. surfaces without thickness) "
            f"{'left out' if drop_open else 'stay open in the STL'}.")

    result = []
    empty_after_cull: list[str] = []
    for s, z0, z1 in bands:
        lz0, lz1 = z0 - origin[2], z1 - origin[2]
        parts = list(pieces[id(s)])
        mp = manifold_pieces[id(s)]
        if mp:
            merged = mf.Manifold.batch_boolean(mp, mf.OpType.Add) if len(mp) > 1 else mp[0]
            if not merged.is_empty():
                parts.append(_from_manifold(merged))
        if not parts:
            continue
        mesh = trimesh.util.concatenate(parts)
        if min_height > 0 and len(mesh.faces):
            bodies = mesh.split(only_watertight=False)
            # only slivers the cut made: flat AND at a cut plane
            def is_sliver(b):
                bz0, bz1 = b.bounds[0][2], b.bounds[1][2]
                at_cut = abs(bz0 - lz0) < 1e-4 or abs(bz1 - lz1) < 1e-4
                return at_cut and (bz1 - bz0) < min_height
            keep = [b for b in bodies if not is_sliver(b)]
            slivers += len(bodies) - len(keep)
            mesh = trimesh.util.concatenate(keep) if keep else None
        if mesh is not None and len(mesh.faces):
            result.append((s, mesh))
        elif parts:
            empty_after_cull.append(s.name)
    if slivers:
        log(f"{slivers} cut slivers below the minimum height removed"
            + (f", story(ies) now empty: {', '.join(empty_after_cull)}." if empty_after_cull else "."))
    return result, origin


def scale_tag(scale_denominator: float) -> str:
    """ Scale for the file name, e.g. 1-500. """
    return f"1-{scale_denominator:g}"


def safe_filename(text: str) -> str:
    return re.sub(r'[<>:"/\\|?*\x00-\x1f]+', "_", text).strip(" ._")


def stl_name_prefix(prefix: str, scale_denominator: float) -> str:
    """ Start of every STL file name: [<prefix>_]<scale>_ """
    pre = safe_filename(prefix)
    return f"{pre + '_' if pre else ''}{scale_tag(scale_denominator)}_"


def write_stls(pieces, out_dir: str, scale_denominator: float, on_bed: bool, log,
               prefix: str = "") -> list[str]:
    """ m -> mm at print scale, one file per story. """
    factor = 1000.0 / scale_denominator
    os.makedirs(out_dir, exist_ok=True)
    written = []
    for n, (storey, mesh) in enumerate(pieces, start=1):
        m = mesh.copy()
        m.apply_scale(factor)
        if on_bed:
            m.apply_translation([0, 0, -m.bounds[0][2]])
        name = safe_filename(storey.name) or "Story"
        path = os.path.join(out_dir, f"{stl_name_prefix(prefix, scale_denominator)}{n:02d}_{name}.stl")
        m.export(path, file_type="stl")
        size = m.extents
        written.append(path)
        # closed = no boundary edges (edges with an odd number of faces) after welding by position
        welded = m.copy()
        welded.merge_vertices()
        _, counts = np.unique(welded.edges_sorted, axis=0, return_counts=True)
        holes = int(np.count_nonzero(counts % 2))
        log(f"  {os.path.basename(path)}  –  {size[0]:.0f} × {size[1]:.0f} × {size[2]:.0f} mm"
            f"{'' if holes == 0 else f'  ({holes} open edges)'}")
    return written


def process(ifc_path, out_dir, scale, offset, union, on_bed, log, progress=None, drop_open=True,
            min_print_mm: float = 0.25, prefix: str = "", ac_stories=None, overrides=None,
            base_plate: bool = False, base_offset: float = 0.0, bottom: float | None = None):
    t = time.time()
    meshes, storeys = load_ifc(ifc_path, log, (lambda p: progress(0.1 + 0.4 * p)) if progress else None)
    if not meshes:
        raise RuntimeError("The IFC file holds no printable geometry.")
    assign_story_indices(storeys, ac_stories, log)
    configure_storeys(storeys, base_plate, base_offset, overrides)
    log("Cutting into stories …")
    pieces, _ = split_into_bands(meshes, storeys, offset, union, log,
                                 (lambda p: progress(0.5 + 0.45 * p)) if progress else None, drop_open,
                                 min_height=min_print_mm * scale / 1000.0, bottom=bottom)
    log(f"Writing {len(pieces)} STL files (1:{scale:g}) …")
    written = write_stls(pieces, out_dir, scale, on_bed, log, prefix)
    log(f"Done in {time.time() - t:.0f} s.")
    return written


# ---------------------------------------------------------------------------
#  Sketch: section diagram of the actual stories
# ---------------------------------------------------------------------------

SK_W, SK_H = 300, 316
SK_BG, SK_EDGE = "#ffffff", "#5c5c5c"
SK_GROUND, SK_LINE, SK_TEXT, SK_HEAD = "#d4d4d4", "#202020", "#303030", "#707070"
SK_FONT = ("Segoe UI", 8)
SK_FONT_B = ("Segoe UI", 8, "bold")
# Colours from Le Corbusier's colour keyboards (Polychromie architecturale)
# Parts: same height = same colour
PART_COLORS = ["#ac4439", "#7ba7ca", "#f2bc1e", "#439070", "#dba3b0", "#3a57a5",
               "#dbb181", "#b8a235", "#c55e3b", "#ddbf99"]
# Cut planes: same offset = same colour (the general offset is always blue);
# a palette of its own, strong colours, so that they stand out from the parts
CUT_COLORS = ["#1f6fe0", "#e0731f", "#2a9d4a", "#c0392b", "#8e44ad", "#138d9e", "#a0522d"]
EXAMPLE_STORIES = [("Basement", -3.0, -1), ("Ground", 0.0, 0), ("Floor 1", 3.0, 1), ("Floor 2", 6.0, 2)]
EXAMPLE_RANGE = (-3.0, 8.6)


def offset_colors(values, general: float) -> dict:
    """ Colour per offset value (rounded to cm); the general offset is blue. """
    out = {round(general, 2): CUT_COLORS[0]}
    for v in values:
        key = round(v, 2)
        if key not in out:
            out[key] = CUT_COLORS[len(out) % len(CUT_COLORS)]
    return out


def height_colors(heights, names=None, memory: dict | None = None) -> dict:
    """ Colour per part height (to the cm): parts of the same height share a colour.
    So that nothing jumps while settings change, `memory` (kept across redraws)
    remembers which height and which part (name) had which colour:
    1. a known height keeps its colour,
    2. else the part keeps its previous colour (e.g. ground floor after an offset change),
    3. else the first free colour.
    The lowest part (its height depends on the bottom) has the fixed colour PART_COLORS[0],
    unless it is exactly as high as another part. """
    out: dict = {}
    if not heights:
        return out
    names = names or [str(i) for i in range(len(heights))]
    mem = memory if memory is not None else {}
    mem_h, mem_n = mem.setdefault("h", {}), mem.setdefault("n", {})
    rest = list(zip(heights[1:], names[1:]))
    taken = set()
    # in three passes, so that a known height comes before everything else
    for source in ("h", "n", None):
        for hv, name in rest:
            if hv in out:
                continue
            if source == "h":
                cand = mem_h.get(hv)
            elif source == "n":
                cand = mem_n.get(name)
            else:
                cand = next((c for c in PART_COLORS[1:] if c not in taken), PART_COLORS[1])
            if cand and cand not in taken and (cand != PART_COLORS[0] or source is None):
                out[hv] = cand
                taken.add(cand)
    for hv, name in rest:
        mem_h[hv] = out[hv]
        mem_n[name] = out[hv]
    if heights[0] not in out:
        out[heights[0]] = PART_COLORS[0]
    return out


def part_heights(bands, zrange):
    zmin, zmax = zrange
    return [round(min(b1, zmax) - max(b0, zmin), 2) for _, b0, b1, _ in bands]


def _lighter(hex_color: str, f: float = 0.35) -> str:
    r, g, b = (int(hex_color[i:i + 2], 16) for i in (1, 3, 5))
    return "#%02x%02x%02x" % tuple(int(c + (255 - c) * f) for c in (r, g, b))


def _darker(hex_color: str, f: float = 0.18) -> str:
    r, g, b = (int(hex_color[i:i + 2], 16) for i in (1, 3, 5))
    return "#%02x%02x%02x" % tuple(int(c * (1 - f)) for c in (r, g, b))


def draw_loading_logo(cv, frame: int) -> None:
    """ Simple loading animation: a quiet grey arc turns. """
    cv.delete("all")
    cv.create_rectangle(0, 0, SK_W, SK_H, fill="#e6e6e6", outline="")
    cx, cy, r = SK_W / 2, SK_H / 2 - 10, 22
    cv.create_oval(cx - r, cy - r, cx + r, cy + r, outline="#d6d6d6", width=5)
    cv.create_arc(cx - r, cy - r, cx + r, cy + r, start=-frame * 12, extent=100,
                  style="arc", outline="#b4b4b4", width=5)
    cv.create_text(cx, cy + r + 22, text="Finding the stories …", fill="#9a9a9a", font=SK_FONT)
    cv.create_rectangle(0, 0, SK_W - 1, SK_H - 1, outline="#a0a0a0")


def draw_section_sketch(cv, stories, zrange, bands, cut_color) -> None:
    """ Schematic section, the stories on the left, the STL parts on the right.
    Ground floor and above: every story equally high, at a fixed position. Below the
    ground floor a fixed scale applies (PPM), so that a deeper bottom only grows
    downwards. Parts of the same height have the same colour, cut planes of the same
    offset the same line colour.
    stories: [(name, level, index)], zrange: (zmin, zmax),
    bands: make_bands(..., with_levels=True), cut_color: offset -> colour. """
    cv.delete("all")
    zmin, zmax = zrange
    top_px, bot_px = 30, SK_H - 52
    eg_i = next((k for k, st in enumerate(stories) if st[2] == 0), None)
    has_eg = eg_i is not None
    eg_i = eg_i or 0
    upper = stories[eg_i:]
    eg_level = upper[0][1]
    basement = has_eg and (eg_i > 0 or zmin < eg_level - 0.01 or
                           any(math.isfinite(b0) and b0 < eg_level - 0.01 for _, b0, _, _ in bands))
    y_eg = top_px + (bot_px - top_px) * (0.66 if basement else 1.0)
    a = [lv for _, lv, _ in upper]
    b = a[1:] + [max(zmax, a[-1] + 0.1)]
    if not has_eg:
        a[0] = min(a[0], zmin)
    h = (y_eg - top_px) / len(upper)
    PPM = 24.0                                   # px per m below the ground floor

    def Y(z):
        if z < a[0]:
            return min(y_eg + (a[0] - z) * PPM, bot_px + 10) if basement else y_eg
        for i in range(len(upper)):
            if z <= b[i]:
                return y_eg - (i + (z - a[i]) / max(b[i] - a[i], 1e-6)) * h
        return top_px

    z_lo = min(zmin, stories[0][1])
    y_box_bot = min(Y(z_lo), bot_px) if basement else y_eg
    x_lab, x_box0, x_box1, x_line1, x_br = 6, 70, 150, 158, 196

    # header
    cv.create_text(x_lab, 12, text="Stories", anchor="w", fill=SK_HEAD, font=SK_FONT_B)
    cv.create_text(x_br - 4, 12, text="Parts (STL)", anchor="w", fill=SK_HEAD, font=SK_FONT_B)
    cv.create_line(x_lab, 21, SK_W - 6, 21, fill="#e0e0e0")

    # ground up to just below the ground floor
    yg = (Y(eg_level) + 2) if has_eg else y_eg
    cv.create_rectangle(0, yg, SK_W, bot_px + 10, fill=SK_GROUND, outline="")

    # colour the parts (same height = same colour)
    heights = part_heights(bands, zrange)
    if not hasattr(cv, "_color_memory"):
        cv._color_memory = {}
    hcol = height_colors(heights, [nm for nm, _, _, _ in bands], cv._color_memory)

    def band_px(b0, b1):
        return (Y(max(b0, z_lo)) if math.isfinite(b0) else Y(z_lo)), Y(min(b1, b[-1]))
    # the lowest part (base plate or ground floor) also colours the surrounding ground
    if bands and has_eg:
        y0, y1 = band_px(bands[0][1], bands[0][2])
        y_hi, y_lo = max(y1, yg), min(y0, bot_px + 10)
        if y_lo > y_hi:
            # lighter: whether a slab or terrain is really modelled there is unknown
            cv.create_rectangle(0, y_hi, SK_W, y_lo,
                                fill=_lighter(hcol[heights[0]], 0.55), outline="")
    for (_, b0, b1, _), hv in zip(bands, heights):
        y0, y1 = band_px(b0, b1)
        cv.create_rectangle(x_box0, y1, x_box1, min(y0, y_box_bot), fill=hcol[hv], outline="")
    if has_eg:
        cv.create_line(0, yg, x_box0, yg, fill=SK_EDGE)
        cv.create_line(x_box1, yg, x_line1 + 30, yg, fill=SK_EDGE)

    # boxes: thin dividing lines, strong outline
    for _, lv, _ in stories[1:]:
        y = Y(lv)
        if top_px < y < y_box_bot:
            cv.create_line(x_box0, y, x_box1, y, fill="#c4c4c4")
    cv.create_rectangle(x_box0, top_px, x_box1, y_box_bot, outline=SK_EDGE)

    # story levels + names on the left
    for i, (name, lv, idx) in enumerate(stories):
        y = Y(max(lv, z_lo))
        if y > bot_px:
            continue                     # deeper than the view
        cv.create_line(x_lab, y, x_line1, y, fill=SK_LINE, dash=(9, 3, 2, 3))
        label = name if len(name) <= 10 else name[:9] + "…"
        cv.create_text(x_lab, y - 6, text=label, anchor="w", fill=SK_TEXT, font=SK_FONT)

    # cut planes in the colour of their offset (lowest: relative to the ground floor)
    label_ys: list[float] = []
    for k, (_, b0, _, level) in enumerate(bands):
        if not math.isfinite(b0):
            continue
        if level is None:
            if k > 0:
                continue
            level = eg_level
        off = round(b0 - level, 2)
        col = cut_color.get(off, CUT_COLORS[0])
        y = Y(b0)
        cv.create_line(x_lab, y, x_line1, y, fill="#ffffff", width=4)     # light halo
        cv.create_line(x_lab, y, x_line1, y, fill=col, width=2)
        ty = y
        while any(abs(ty - other) < 9 for other in label_ys):   # labels must not overlap
            ty += 9 if k == 0 else -9
        label_ys.append(ty)
        cv.create_text(x_line1 + 2, ty, text=f"{off:+.2f}", anchor="w", fill=col, font=SK_FONT)

    # parts on the right: bracket, colour swatch, number + name
    for k, ((name, b0, b1, _), hv) in enumerate(zip(bands, heights)):
        yb, yt = band_px(b0, b1)
        yb = min(yb, bot_px)
        cv.create_line(x_br, yb - 1, x_br, yt + 1, fill=SK_EDGE)
        cv.create_line(x_br - 3, yb - 1, x_br + 3, yb - 1, fill=SK_EDGE)
        cv.create_line(x_br - 3, yt + 1, x_br + 3, yt + 1, fill=SK_EDGE)
        if yb - yt >= 10:
            ym = (yb + yt) / 2
            cv.create_rectangle(x_br + 6, ym - 4, x_br + 14, ym + 4, fill=hcol[hv], outline=SK_EDGE)
            label = f"{k + 1:02d} {name}"
            cv.create_text(x_br + 18, ym, text=label if len(label) <= 15 else label[:14] + "…",
                           anchor="w", fill=SK_TEXT, font=SK_FONT)

    # legend
    y = SK_H - 14
    cv.create_rectangle(0, SK_H - 42, SK_W, SK_H, fill=SK_BG, outline="")
    cv.create_line(8, y, 30, y, fill=SK_LINE, dash=(9, 3, 2, 3))
    cv.create_text(34, y, text="Story level", anchor="w", fill=SK_TEXT, font=SK_FONT)
    cv.create_line(112, y, 134, y, fill=CUT_COLORS[0], width=2)
    cv.create_text(138, y, text="Cut (colour = offset)", anchor="w", fill=SK_TEXT, font=SK_FONT)
    cv.create_text(SK_W - 6, SK_H - 32, text="schematic, not to scale", anchor="e",
                   fill="#999", font=("Segoe UI", 7))
    cv.create_rectangle(0, 0, SK_W - 1, SK_H - 1, outline="#a0a0a0")


# ---------------------------------------------------------------------------
#  Dialog
# ---------------------------------------------------------------------------

def run_dialog(ac: Archicad):
    import queue
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    if sys.platform == "win32":
        try:
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception:
            pass

    # checks first – errors as Archicad alerts
    try:
        if ac.tapir_version() < MIN_TAPIR_VERSION:
            ac.alert("error", "The Tapir Add-On is too old.",
                     "Please install Tapir 1.6.0 or later.")
            return
        if ac.current_window_type() != "3DModel":
            ac.alert("warning", "Please open the 3D window first.",
                     "The script exports what is visible in the 3D window.")
            return
        translators = ac.translators()
        project = ac.project_name()
        ac_stories = ac.stories()
    except ArchicadError as e:
        print(e)
        ac.alert("error", "Could not connect to Archicad.", str(e))
        return
    if not translators:
        ac.alert("error", "No IFC translator found.",
                 "The script needs an IFC export translator (File > Interoperability > IFC > IFC Translators).")
        return

    root = tk.Tk()
    root.title(f"{SCRIPT_TITLE} – {project}" if project else SCRIPT_TITLE)
    root.resizable(False, False)
    style = ttk.Style(root)
    if "vista" in style.theme_names():
        style.theme_use("vista")
    root.attributes("-topmost", True)
    root.after(500, lambda: root.attributes("-topmost", False))
    hint = dict(foreground="#666", justify="left")

    frm = ttk.Frame(root, padding=14)
    frm.grid(sticky="nsew")
    row = 0

    v_dir = tk.StringVar(value=str(default_output_folder(project)))
    v_scale = tk.StringVar(value="500")
    v_offset = tk.StringVar(value="-0.30")
    v_base = tk.BooleanVar(value=False)
    v_base_off = tk.StringVar(value="0.00")
    v_indiv = tk.BooleanVar(value=False)
    v_source = tk.StringVar(value="visible")
    v_trans = tk.StringVar(value=translators[0])
    v_union = tk.BooleanVar(value=True)
    v_bottom = tk.BooleanVar(value=True)
    v_bottom_off = tk.StringVar(value="0.80")       # depth below the ground floor (positive = down)
    v_min = tk.StringVar(value="0.25")
    v_label = tk.StringVar(value="")
    v_status = tk.StringVar(value="Ready.")

    def num(var, default=0.0):
        try:
            return float(var.get().replace(",", "."))
        except ValueError:
            return default

    def swatch(parent):
        return tk.Canvas(parent, width=22, height=10, highlightthickness=0, bg=root.cget("bg"))

    # --- Elements --------------------------------------------------------------
    grp = ttk.LabelFrame(frm, text="Elements", padding=(10, 6))
    grp.grid(row=row, column=0, sticky="ew"); row += 1
    ttk.Radiobutton(grp, text="Visible in the 3D window (isolated)", value="visible",
                    variable=v_source).grid(row=0, column=0, sticky="w")
    ttk.Radiobutton(grp, text="Current selection", value="selection",
                    variable=v_source).grid(row=1, column=0, sticky="w")

    # --- Print -------------------------------------------------------------------
    grp2 = ttk.LabelFrame(frm, text="Print", padding=(10, 6))
    grp2.grid(row=row, column=0, sticky="ew", pady=(8, 0)); row += 1
    opts = ttk.Frame(grp2)
    opts.grid(row=0, column=0, sticky="nw")

    def spin(parent, var, lo, hi, step, fmt="%.2f"):
        return ttk.Spinbox(parent, textvariable=var, width=7, from_=lo, to=hi, increment=step, format=fmt)

    r = 0
    ttk.Label(opts, text="Scale 1:").grid(row=r, column=0, sticky="w")
    ttk.Combobox(opts, textvariable=v_scale, width=7,
                 values=["100", "200", "250", "500", "1000"]).grid(row=r, column=1, sticky="w", padx=6)
    r += 1
    ttk.Label(opts, text="Cut offset from story level [m]:").grid(row=r, column=0, sticky="w", pady=(8, 0))
    sp_offset = spin(opts, v_offset, -3.0, 3.0, 0.05)
    sp_offset.grid(row=r, column=1, sticky="w", padx=6, pady=(8, 0))
    sw_offset = swatch(opts)
    sw_offset.grid(row=r, column=2, sticky="w", pady=(8, 0)); r += 1
    ttk.Label(opts, text="Recommended: −0.30 m. Negative = below the\nstory level, positive = above it.",
              **hint).grid(row=r, column=0, columnspan=2, sticky="w"); r += 1
    ttk.Checkbutton(opts, text="Set each cut plane individually", variable=v_indiv).grid(
        row=r, column=0, columnspan=2, sticky="w", pady=(2, 0)); r += 1

    ttk.Checkbutton(opts, text="Separate base plate (cut below ground floor) [m]:", variable=v_base).grid(
        row=r, column=0, sticky="w", pady=(8, 0))
    sp_base = spin(opts, v_base_off, -3.0, 3.0, 0.05)
    sp_base.grid(row=r, column=1, sticky="w", padx=6, pady=(8, 0))
    base_spins = [sp_base]
    base_swatches = [swatch(opts)]
    base_swatches[0].grid(row=r, column=2, sticky="w", pady=(8, 0)); r += 1
    ttk.Label(opts, text="Everything below this cut becomes the base plate.\n"
                         "Unticked, basement stories belong to the ground floor.",
              **hint).grid(row=r, column=0, columnspan=2, sticky="w"); r += 1
    ttk.Checkbutton(opts, text="Bottom of lowest part (depth below ground floor) [m]:", variable=v_bottom).grid(
        row=r, column=0, sticky="w", pady=(6, 0))
    sp_bottom = spin(opts, v_bottom_off, -3.0, 20.0, 0.05)
    sp_bottom.grid(row=r, column=1, sticky="w", padx=6, pady=(6, 0))
    sw_bottom = swatch(opts)
    sw_bottom.grid(row=r, column=2, sticky="w", pady=(6, 0)); r += 1
    ttk.Label(opts, text="Last cut plane: nothing below it is exported\n"
                         "(e.g. the underside of the base plate).",
              **hint).grid(row=r, column=0, columnspan=2, sticky="w"); r += 1

    ttk.Label(opts, text="Minimum height [mm]:").grid(row=r, column=0, sticky="w", pady=(8, 0))
    spin(opts, v_min, 0.0, 20.0, 0.05).grid(row=r, column=1, sticky="w", padx=6, pady=(8, 0)); r += 1
    ttk.Label(opts, text="Thinner remains at the cuts are removed or\nsnapped to the cut plane. "
                         "Surfaces without\nthickness are always left out. The parts\nlie on the print bed (Z = 0).",
              **hint).grid(row=r, column=0, columnspan=2, sticky="w"); r += 1
    ttk.Checkbutton(opts, text="Union each story into one body",
                    variable=v_union).grid(row=r, column=0, columnspan=2, sticky="w", pady=(8, 0)); r += 1

    sketch = tk.Canvas(grp2, width=SK_W, height=SK_H, bg=SK_BG, highlightthickness=0)
    sketch.grid(row=0, column=1, sticky="ne", padx=(14, 0))

    indiv = ttk.Frame(grp2)
    indiv.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(10, 0))
    indiv.grid_remove()

    # --- state: the stories the elements touch -----------------------------------
    state = {"relevant": [], "zrange": None}
    rows: list[dict] = []      # {"index", "name", "level", "cut": BooleanVar, "off": StringVar}

    def overrides():
        if not v_indiv.get():
            return None
        return {rw["index"]: (rw["cut"].get(), num(rw["off"], num(v_offset))) for rw in rows}

    def current_bands(stories):
        sts = [Storey(name, lv, index=idx) for name, lv, idx in stories]
        configure_storeys(sts, v_base.get(), num(v_base_off), overrides())
        return make_bands(sts, num(v_offset), with_levels=True,
                          bottom=-num(v_bottom_off) if v_bottom.get() else None)

    def paint(c, color):
        c.delete("all")
        if color:
            c.create_line(1, 5, 21, 5, fill=color, width=3)

    def redraw(*_):
        if state.get("loading"):
            return
        stories, zr = state["relevant"], state["zrange"]
        if not stories or zr is None:
            stories, zr = EXAMPLE_STORIES, EXAMPLE_RANGE
        bands = current_bands(stories)
        # leave out parts outside the model
        bands = [b for b in bands if b[2] > zr[0] + 1e-6 and b[1] < zr[1] - 1e-6]
        g = num(v_offset)
        values = [num(v_base_off)] if v_base.get() else []
        if v_bottom.get():
            values.append(-num(v_bottom_off))
        if v_indiv.get():
            values += [num(rw["off"], g) for rw in rows if rw["cut"].get()]
        colors = offset_colors(values, g)
        draw_section_sketch(sketch, stories, zr, bands, colors)
        uses_general = not v_indiv.get() or any(
            rw["cut"].get() and round(num(rw["off"], g), 2) == round(g, 2) for rw in rows)
        paint(sw_offset, colors[round(g, 2)] if uses_general else None)
        for c in base_swatches:
            paint(c, colors.get(round(num(v_base_off), 2)) if v_base.get() else None)
        paint(sw_bottom, colors.get(round(-num(v_bottom_off), 2)) if v_bottom.get() else None)
        for rw in rows:
            paint(rw["sw"], colors.get(round(num(rw["off"], g), 2)) if rw["cut"].get() else None)

    def update_relevant():
        """ Finds in the background which stories the elements touch; meanwhile
        the sketch shows the loading animation. """
        if state.get("loading"):
            state["reload"] = True
            return
        state["loading"], state["reload"] = True, False
        source = v_source.get()
        animate(0)

        def work():
            err, zr = None, None
            try:
                elements = ac.selected() if source == "selection" else ac.visible_in_3d()
                zr = ac.z_range(elements) if elements else None
            except Exception as e:          # Archicad not reachable or similar
                err = e
            ui(finish, zr, err)
        threading.Thread(target=work, daemon=True).start()

    def finish(zr, err):
        state["loading"] = False
        if err is not None:
            v_status.set(f"Could not find the stories: {err}")
        state["zrange"] = zr
        if zr is None:
            state["relevant"] = []
        else:
            levels = [lv for _, lv, _ in ac_stories]
            state["relevant"] = [ac_stories[i] for i, lv in enumerate(levels)
                                 if (levels[i + 1] if i + 1 < len(levels) else math.inf) > zr[0]
                                 and lv < zr[1]]
        build_rows()
        redraw()
        if state.get("reload"):
            update_relevant()

    def animate(frame):
        if not state.get("loading"):
            return
        draw_loading_logo(sketch, frame)
        root.after(40, lambda: animate(frame + 1))

    def build_rows():
        for w in indiv.winfo_children():
            w.destroy()
        rows.clear()
        del base_swatches[1:]
        del base_spins[1:]
        stories = state["relevant"]
        cand = [st for k, st in enumerate(stories) if st[2] >= 1 and k > 0]
        has_eg = any(st[2] == 0 for st in stories)
        bold = ("Segoe UI", 9, "bold")
        ttk.Label(indiv, text="Cut", font=bold).grid(row=0, column=0, sticky="w")
        ttk.Label(indiv, text="below story", font=bold).grid(row=0, column=1, sticky="w", padx=6)
        ttk.Label(indiv, text="Level", font=bold).grid(row=0, column=2, sticky="e", padx=6)
        ttk.Label(indiv, text="Offset [m]", font=bold).grid(row=0, column=3, columnspan=2, sticky="w", padx=6)
        r_ = 1
        for name, lv, idx in reversed(cand):                    # top story first
            v_cut, v_off = tk.BooleanVar(value=True), tk.StringVar(value=f"{num(v_offset):.2f}")
            sp = spin(indiv, v_off, -3.0, 3.0, 0.05)
            def toggled(sp=sp, v=v_cut):
                sp.state(["!disabled" if v.get() else "disabled"])
                redraw()
            ttk.Checkbutton(indiv, variable=v_cut, command=toggled).grid(row=r_, column=0, sticky="w")
            ttk.Label(indiv, text=name).grid(row=r_, column=1, sticky="w", padx=6)
            ttk.Label(indiv, text=f"{lv:+.2f}").grid(row=r_, column=2, sticky="e", padx=6)
            sp.grid(row=r_, column=3, sticky="w", padx=6, pady=1)
            sw = swatch(indiv)
            sw.grid(row=r_, column=4, sticky="w")
            v_off.trace_add("write", redraw)
            rows.append({"index": idx, "name": name, "level": lv, "cut": v_cut, "off": v_off, "sw": sw})
            r_ += 1
        if has_eg:
            name, lv, _ = next(st for st in stories if st[2] == 0)
            sp = spin(indiv, v_base_off, -3.0, 3.0, 0.05)
            ttk.Checkbutton(indiv, variable=v_base).grid(row=r_, column=0, sticky="w")
            ttk.Label(indiv, text=f"{name}  (below: base plate)").grid(row=r_, column=1, sticky="w", padx=6)
            ttk.Label(indiv, text=f"{lv:+.2f}").grid(row=r_, column=2, sticky="e", padx=6)
            sp.grid(row=r_, column=3, sticky="w", padx=6, pady=1)
            sw = swatch(indiv)
            sw.grid(row=r_, column=4, sticky="w")
            base_spins.append(sp)
            base_swatches.append(sw)
            r_ += 1
        if not cand and not has_eg:
            ttk.Label(indiv, text="No cuts within the height of the elements.",
                      **hint).grid(row=r_, column=0, columnspan=5, sticky="w")
            return
        ttk.Label(indiv, text="Unticked: no cut, the story stays joined to the one below.",
                  **hint).grid(row=r_, column=0, columnspan=5, sticky="w", pady=(4, 0))
        toggle_base()

    def toggle_indiv(*_):
        if v_indiv.get():
            g = f"{num(v_offset):.2f}"            # start from the general value
            for rw in rows:
                rw["off"].set(g)
            indiv.grid()
            sp_offset.state(["disabled"])
        else:
            indiv.grid_remove()
            sp_offset.state(["!disabled"])
        redraw()

    def toggle_base(*_):
        for sp in base_spins:
            sp.state(["!disabled" if v_base.get() else "disabled"])
        redraw()

    v_indiv.trace_add("write", toggle_indiv)
    v_base.trace_add("write", toggle_base)
    v_offset.trace_add("write", redraw)
    v_base_off.trace_add("write", redraw)
    v_bottom_off.trace_add("write", redraw)

    def toggle_bottom(*_):
        sp_bottom.state(["!disabled" if v_bottom.get() else "disabled"])
        redraw()
    v_bottom.trace_add("write", toggle_bottom)
    v_source.trace_add("write", lambda *_: update_relevant())

    # --- Output ------------------------------------------------------------------
    grp3 = ttk.LabelFrame(frm, text="Output", padding=(10, 6))
    grp3.grid(row=row, column=0, sticky="ew", pady=(8, 0)); row += 1
    ttk.Label(grp3, text="IFC translator:").grid(row=0, column=0, sticky="w")
    ttk.Combobox(grp3, textvariable=v_trans, values=translators, state="readonly",
                 width=34).grid(row=0, column=1, columnspan=2, sticky="w", padx=6)
    ttk.Label(grp3, text="Part / variant:").grid(row=1, column=0, sticky="w", pady=(4, 0))
    ttk.Entry(grp3, textvariable=v_label, width=24).grid(row=1, column=1, sticky="w", padx=6, pady=(4, 0))
    ttk.Label(grp3, text="optional", **hint).grid(row=1, column=2, sticky="w", pady=(4, 0))
    ttk.Label(grp3, text="Output folder:").grid(row=2, column=0, sticky="w", pady=(4, 0))
    ttk.Entry(grp3, textvariable=v_dir, width=48).grid(row=2, column=1, sticky="w", padx=6, pady=(4, 0))

    def browse():
        d = filedialog.askdirectory(parent=root, initialdir=v_dir.get() or str(Path.home()))
        if d:
            v_dir.set(os.path.normpath(d))
    ttk.Button(grp3, text="Browse …", command=browse).grid(row=2, column=2, sticky="w", pady=(4, 0))
    v_example = tk.StringVar()
    ttk.Label(grp3, textvariable=v_example, **hint).grid(row=3, column=1, columnspan=2,
                                                        sticky="w", padx=6, pady=(4, 0))

    def file_prefix() -> str:
        return "_".join(x for x in (safe_filename(project), safe_filename(v_label.get())) if x)

    def update_example(*_):
        sc = num(v_scale, 0)
        start_ = stl_name_prefix(file_prefix(), sc) if sc > 0 else f"{file_prefix()}_1-?_"
        v_example.set(f"File name: {start_}01_<story>.stl")
    v_label.trace_add("write", update_example)
    v_scale.trace_add("write", update_example)
    update_example()

    bar = ttk.Progressbar(frm, mode="determinate", maximum=1.0, length=420)
    bar.grid(row=row, column=0, sticky="ew", pady=(12, 2)); row += 1
    ttk.Label(frm, textvariable=v_status, width=70).grid(row=row, column=0, sticky="w"); row += 1

    btns = ttk.Frame(frm)
    btns.grid(row=row, column=0, sticky="e", pady=(10, 0))
    b_run = ttk.Button(btns, text="Export", default="active")
    b_close = ttk.Button(btns, text="Close", command=root.destroy)
    b_run.grid(row=0, column=0, padx=(0, 6)); b_close.grid(row=0, column=1)

    # The export runs in a background thread; only the main thread may touch Tk,
    # so all UI changes go through a queue.
    ui_queue: queue.Queue = queue.Queue()

    def ui(fn, *a, **kw):
        ui_queue.put(lambda: fn(*a, **kw))

    def pump():
        try:
            while True:
                ui_queue.get_nowait()()
        except queue.Empty:
            pass
        root.after(50, pump)
    root.after(50, pump)

    def log(msg):
        print(msg, flush=True)
        ui(v_status.set, msg.strip())

    def set_progress(p):
        ui(bar.configure, value=p)

    def worker(params):
        ok, msg = False, ""
        try:
            with tempfile.TemporaryDirectory(prefix="ac_stl_") as tmp:
                ifc_path = os.path.join(tmp, "export.ifc")
                export_ifc_from_archicad(ac, params["source"], params["translator"], ifc_path, log)
                set_progress(0.1)
                files = process(ifc_path, params["out"], params["scale"], params["offset"],
                                params["union"], True, log, set_progress, True,
                                params["min_mm"], params["prefix"], ac_stories, params["overrides"],
                                params["base"], params["base_off"], params["bottom"])
            set_progress(1.0)
            ok = True
            msg = f"{len(files)} STL files exported – {params['out']}"
        except Exception as e:
            traceback.print_exc()
            msg = f"Export failed: {e}"

        def done():
            b_run.state(["!disabled"]); b_close.state(["!disabled"])
            v_status.set(msg)
            if not ok:
                ac.alert("error", "Export failed.", msg)
        ui(done)

    def start():
        try:
            scale = float(v_scale.get().replace(",", "."))
            offset = float(v_offset.get().replace(",", "."))
            base_off = float(v_base_off.get().replace(",", "."))
            min_mm = float(v_min.get().replace(",", "."))
            assert scale > 0 and min_mm >= 0
        except Exception:
            v_status.set("Please enter valid values for scale, offset and minimum height.")
            return
        out = v_dir.get().strip()
        if not out:
            v_status.set("Please choose an output folder.")
            return
        # never overwrite STL files of an earlier export without asking
        pattern = glob.escape(stl_name_prefix(file_prefix(), scale)) + "*.stl"
        existing = sorted(Path(out).glob(pattern)) if Path(out).is_dir() else []
        if existing and not messagebox.askyesno(
                SCRIPT_TITLE,
                f"{len(existing)} STL file(s) from an earlier export with the same scale are in\n{out}\n\n"
                "Files with the same names will be replaced. Continue?",
                icon="warning", parent=root):
            return
        params = dict(source=v_source.get(), translator=v_trans.get(), out=out,
                      scale=scale, offset=offset, union=v_union.get(), min_mm=min_mm,
                      prefix=file_prefix(), bottom=-num(v_bottom_off) if v_bottom.get() else None,
                      overrides=overrides(), base=v_base.get(), base_off=base_off)
        v_status.set(f"Exporting STL files to {out} …")
        b_run.state(["disabled"]); b_close.state(["disabled"])
        bar.configure(value=0)
        threading.Thread(target=worker, args=(params,), daemon=True).start()

    b_run.configure(command=start)
    root.bind("<Return>", lambda e: start())
    root.bind("<Escape>", lambda e: root.destroy())
    toggle_base()
    toggle_bottom()
    root.after(100, update_relevant)
    root.mainloop()


# ---------------------------------------------------------------------------

def main():
    p = argparse.ArgumentParser(description=SCRIPT_TITLE)
    p.add_argument("--port", type=int, default=19723)
    p.add_argument("--ifc", help="Post-processing only: process an existing IFC file (without Archicad)")
    p.add_argument("--out", help="Output folder (with --ifc)")
    p.add_argument("--scale", type=float, default=500)
    p.add_argument("--offset", type=float, default=-0.30)
    p.add_argument("--no-union", action="store_true")
    p.add_argument("--keep-open", action="store_true")
    p.add_argument("--base-plate", type=float, default=None, metavar="OFFSET",
                   help="Separate base plate: cut below the ground floor at this offset [m]")
    p.add_argument("--bottom", type=float, default=None, metavar="M",
                   help="Bottom of the lowest part: depth below the ground floor [m]; nothing below is exported")
    p.add_argument("--min-mm", type=float, default=0.25, help="Minimum height of a part in the print [mm]")
    args, _ = p.parse_known_args()

    if args.ifc:
        out = args.out or os.path.splitext(args.ifc)[0] + "_STL"
        prefix = os.path.splitext(os.path.basename(args.ifc))[0]
        process(args.ifc, out, args.scale, args.offset, not args.no_union, True, print,
                drop_open=not args.keep_open, min_print_mm=args.min_mm, prefix=prefix,
                base_plate=args.base_plate is not None, base_offset=args.base_plate or 0.0,
                bottom=-args.bottom if args.bottom is not None else None)
        return
    run_dialog(Archicad(args.port))


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()
