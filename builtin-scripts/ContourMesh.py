# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
ContourMesh

Reads a DXF file with 3D contour lines and creates a single native ArchiCAD
Mesh from them. The outer boundary is the convex hull of all the points with
its Z interpolated to the ground, and every contour ring becomes a subline at
its own elevation, so the mesh reads as a 3D terrain with contour ridge lines.

The mesh is placed at its raw DXF coordinates by default; you can relocate it
to the project origin (0, 0, 0) or to a specific coordinate, override the unit
system the DXF coordinates are in (the header's $INSUNITS is often wrong), and
set the height of the skirt below the base level.

Copyright (c) 2026 Paul K. All rights reserved.
"""

import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import run_tapir
from utilities.ui import ScriptWindow, require_archicad

SCRIPT_NAME = "ContourMesh"
DEFAULT_SKIRT = 1.0
UNIT_VALUES = ["auto", "meters", "millimeters", "centimeters", "feet", "inches", "kilometers"]


def convex_hull(points):
    """Andrew's monotone chain convex hull. Returns hull in CCW order."""
    pts = sorted(set(points), key=lambda p: (p[0], p[1]))
    if len(pts) <= 2:
        return pts
    def cross(o, a, b):
        return (a[0]-o[0])*(b[1]-o[1]) - (a[1]-o[1])*(b[0]-o[0])
    lower = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def ring_area(ring):
    a = 0.0; n = len(ring)
    for i in range(n):
        x1,y1,_ = ring[i]; x2,y2,_ = ring[(i+1)%n]
        a += x1*y2 - x2*y1
    return abs(a)/2.0


def dedup_ring(ring, tol=0.001):
    """Remove consecutive duplicate/coincident points from a ring."""
    if len(ring) < 2:
        return ring
    out = [ring[0]]
    for p in ring[1:]:
        prev = out[-1]
        dx, dy, dz = p[0]-prev[0], p[1]-prev[1], p[2]-prev[2]
        if dx*dx + dy*dy + dz*dz > tol*tol:
            out.append(p)
    # Remove last point if it coincides with first (closed ring)
    if len(out) > 1:
        first, last = out[0], out[-1]
        dx, dy, dz = last[0]-first[0], last[1]-first[1], last[2]-first[2]
        if dx*dx + dy*dy + dz*dz <= tol*tol:
            out.pop()
    return out


def parse_dxf(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    lines = content.splitlines()
    pairs = []
    i = 0
    while i + 1 < len(lines):
        pairs.append((lines[i].strip(), lines[i + 1]))
        i += 2
    rings = []
    cur_entity = None
    cur_pts = []
    cur_closed = False
    cur_elev = 0.0
    def flush():
        nonlocal cur_pts, cur_closed, cur_elev
        if len(cur_pts) >= 3:
            # Apply elevation to all points if they're all at z=0
            if all(abs(p[2]) < 1e-10 for p in cur_pts) and abs(cur_elev) > 1e-10:
                cur_pts = [(p[0], p[1], cur_elev) for p in cur_pts]
            rings.append(cur_pts[:])
        cur_pts = []
        cur_closed = False
        cur_elev = 0.0
    for idx, (code, val) in enumerate(pairs):
        if code == '0':
            flush()
            etype = val.strip()
            cur_entity = etype if etype in ('LWPOLYLINE', 'POLYLINE') else None
            continue
        if cur_entity is None:
            continue
        if cur_entity == 'LWPOLYLINE':
            if code == '70':
                try: cur_closed = (int(val.strip()) & 1) == 1
                except: pass
            elif code == '38':
                try: cur_elev = float(val.strip())
                except: pass
            elif code == '10':
                x = float(val.strip())
                y = 0.0; z = 0.0
                if idx+1 < len(pairs) and pairs[idx+1][0] == '20':
                    y = float(pairs[idx+1][1].strip())
                if idx+2 < len(pairs) and pairs[idx+2][0] == '23':
                    z = float(pairs[idx+2][1].strip())
                elif idx+1 < len(pairs) and pairs[idx+1][0] == '23':
                    z = float(pairs[idx+1][1].strip())
                cur_pts.append((x, y, z))
    flush()
    return rings
def read_dxf_units(path):
    """Read the $INSUNITS value from a DXF file's header.

    Returns the unit code as an int, or 0 if unspecified/missing.
    DXF $INSUNITS codes:
      0 = unspecified, 1 = inches, 2 = feet, 3 = miles,
      4 = millimeters, 5 = centimeters, 6 = meters,
      7 = decimeters, 8 = decameters, 9 = hectometers, 10 = kilometers
    """
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            lines = f.read(50000).splitlines()
        for i in range(len(lines) - 2):
            if lines[i].strip() == '$INSUNITS':
                # Format: $INSUNITS / 70 / <value>
                if lines[i+1].strip() == '70':
                    return int(lines[i+2].strip())
    except Exception:
        pass
    return 0


def dxf_units_to_meters(unit_code):
    """Convert a DXF $INSUNITS code to a scale factor (DXF units -> meters)."""
    factors = {
        0: 1.0,      # unspecified -> assume meters
        1: 0.0254,   # inches
        2: 0.3048,   # feet
        3: 1609.344, # miles
        4: 0.001,    # millimeters
        5: 0.01,     # centimeters
        6: 1.0,      # meters
        7: 0.1,      # decimeters
        8: 10.0,     # decameters
        9: 100.0,    # hectometers
        10: 1000.0,  # kilometers
    }
    return factors.get(unit_code, 1.0)


UNIT_NAMES = {
    "meters": 6, "metre": 6, "metres": 6, "m": 6,
    "millimeters": 4, "millimetres": 4, "mm": 4,
    "feet": 2, "ft": 2, "foot": 2,
    "inches": 1, "in": 1, "inch": 1,
    "kilometers": 10, "kilometres": 10, "km": 10,
    "centimeters": 5, "centimetres": 5, "cm": 5,
}
UNIT_NAMES_REV = {0: "unspecified (assumed meters)", 1: "inches", 2: "feet",
                  3: "miles", 4: "millimeters", 5: "centimeters", 6: "meters",
                  7: "decimeters", 8: "decameters", 9: "hectometers", 10: "kilometers"}


def _nearest_z_tree(rings):
    """Build a KD-tree over all contour points for fast nearest-Z lookup.

    Returns (tree, z_array) where tree.query([x,y]) gives the index of the
    nearest contour point and z_array[idx] is its elevation. Used to assign a
    plausible Z to the convex-hull outer boundary so the terrain base follows
    the ground instead of sitting flat at Z=0.
    """
    try:
        import numpy as np
        from scipy.spatial import cKDTree
        pts = np.array([[p[0], p[1]] for r in rings for p in r], dtype=float)
        zs = np.array([p[2] for r in rings for p in r], dtype=float)
        return cKDTree(pts), zs
    except Exception:
        return None, None


def _hull_outer_with_z(rings, tree, zs, scale, offset_x, offset_y, offset_z):
    """Convex-hull outer boundary with Z interpolated from the nearest contour.

    The convex hull of all points is always a simple (non-self-intersecting)
    polygon, which is the one hard requirement Tapir imposes on the outer ring.
    Each hull vertex is given the Z of the nearest contour point so the base
    of the terrain tracks the ground surface.
    """
    hull = convex_hull([(p[0], p[1]) for r in rings for p in r])
    outer = []
    for hx, hy in hull:
        z = 0.0
        if tree is not None:
            try:
                _, idx = tree.query([hx, hy])
                z = float(zs[idx])
            except Exception:
                z = 0.0
        outer.append({"x": hx*scale + offset_x, "y": hy*scale + offset_y, "z": z*scale + offset_z})
    return outer
def rings_to_tapir_mesh(rings, offset_x=0.0, offset_y=0.0, offset_z=0.0, scale=1.0, min_area=1.0, skirt_height=DEFAULT_SKIRT):
    """Build a SINGLE triangulated terrain mesh from contour rings.

    Approach (verified against ArchiCAD 30 / Tapir source):
      * outer boundary  = convex hull of all points, Z interpolated to the
                          ground (always a simple polygon -> Tapir accepts it)
      * every contour ring = a subline at its own elevation

    Tapir's CreateMeshes reads each subline's 3D coordinates (including Z) via
    Get3DCoordinateFromObjectState and stores them in meshLevelCoords, so the
    contour rings become real 3D ridge lines that deform the mesh surface.

    CRITICAL: each subline must be an object {"coordinates": [pts]} — NOT a
    bare array. Bare arrays are ignored (nSubLines stays 0).

    Self-intersecting rings are fine as sublines; only the outer ring must be
    simple, which the convex hull guarantees.

    skirt_height: the height of the mesh's skirt below the base level, in
    METERS. Must be >= 0 (negative values corrupt the mesh). The mesh is
    created as a SolidBodyWithSkirt. Default is 1.0 meter.
    """
    if not rings:
        raise ValueError("No rings")
    rings = [dedup_ring(r) for r in rings]
    # Filter degenerate rings. min_area is in SQUARE METERS, so compare the
    # ring's area against min_area/scale^2 in raw DXF units. This keeps the
    # filter consistent regardless of the unit system (a 1 m^2 ring is a
    # 1 m^2 ring whether the DXF is in meters, inches, or feet).
    min_area_raw = min_area / (scale * scale) if scale else min_area
    rings = [r for r in rings if len(r) >= 3 and ring_area(r) >= min_area_raw]
    if not rings:
        raise ValueError("All rings degenerate after dedup")

    tree, zs = _nearest_z_tree(rings)
    outer = _hull_outer_with_z(rings, tree, zs, scale, offset_x, offset_y, offset_z)
    if len(outer) < 3:
        raise ValueError(f"Convex hull has {len(outer)} pts")

    def pt(p):
        return {"x": p[0]*scale + offset_x, "y": p[1]*scale + offset_y, "z": p[2]*scale + offset_z}

    # CRITICAL: Tapir expects each subline to be an OBJECT with a "coordinates"
    # key, NOT a bare array. The 3D coordinate (including Z) of each point is
    # read via Get3DCoordinateFromObjectState, so the subline's Z values create
    # real 3D relief (the contour ridge lines). Passing bare arrays makes Tapir
    # ignore the sublines entirely (nSubLines stays 0).
    sublines = [{"coordinates": [pt(p) for p in r]} for r in rings]
    mesh = {"polygonCoordinates": outer}
    if sublines:
        mesh["sublines"] = sublines
    # showLines makes ArchiCAD render the mesh edges (the contour ridge lines)
    # on top of the surface, so the terrain reads as contour lines rather than
    # a single solid plane.
    mesh["showLines"] = True
    # Skirt: create a solid body with a skirt that extends below the base.
    # skirt_height is in meters and MUST be >= 0 (negative values corrupt the
    # mesh — verified: a negative skirt pushes the top surface up).
    #
    # The skirt extends downward from the mesh's base level (the convex hull).
    # To ensure it reaches below the lowest contour point (which may be an
    # interior point below the hull), add the depth below zero to the skirt
    # height. This guarantees the skirt extends at least to the lowest contour
    # plus the user's extra depth.
    skirt_m = max(0.0, float(skirt_height))
    if skirt_m > 0:
        # Find the lowest point Z (in meters, after scaling).
        min_z_m = min(p[2] for r in rings for p in r) * scale + offset_z
        # If the lowest point is below zero, add its depth to the skirt height.
        below_zero_depth = max(0.0, -min_z_m)
        mesh["skirtType"] = "SolidBodyWithSkirt"
        mesh["skirtLevel"] = skirt_m + below_zero_depth
    return mesh
class ContourMesh:
    """ The Archicad side: pick a DXF, choose the options, create the mesh. """

    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Creates a single 3D terrain mesh from a DXF with 3D contour lines. "
            "Pick the file, choose the placement and units, then press Create mesh.",
            min_width=480, min_height=420)
        self.dxf_path = ""
        self._build_body()
        self.window.add_button("Create mesh", self.create_mesh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.columnconfigure(0, weight=1)

        file_frame = ttk.LabelFrame(body, text="DXF file", padding=(10, 6))
        file_frame.grid(row=0, column=0, sticky="ew")
        file_frame.columnconfigure(1, weight=1)
        self.file_var = tk.StringVar()
        ttk.Entry(file_frame, textvariable=self.file_var).grid(row=0, column=0, columnspan=2, sticky="ew")
        ttk.Button(file_frame, text="Browse...", command=self._browse).grid(row=0, column=2, padx=(6, 0))

        units_frame = ttk.LabelFrame(body, text="Units (what are the DXF coordinates in?)", padding=(10, 6))
        units_frame.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        units_frame.columnconfigure(1, weight=1)
        self.units_var = tk.StringVar(value="auto")
        ttk.Combobox(units_frame, textvariable=self.units_var, values=UNIT_VALUES,
                     state="readonly").grid(row=0, column=0, columnspan=2, sticky="ew")
        ttk.Label(units_frame, text="Pick 'auto' to trust the DXF header; otherwise set the real unit.",
                  style="Muted.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(4, 0))

        placement_frame = ttk.LabelFrame(body, text="Placement", padding=(10, 6))
        placement_frame.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        placement_frame.columnconfigure(1, weight=1)
        self.origin_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(placement_frame, text="Relocate mesh to project origin (0, 0, 0)",
                        variable=self.origin_var).grid(row=0, column=0, columnspan=2, sticky="w")
        ttk.Label(placement_frame, text="Target location (optional, overrides origin):",
                  style="Muted.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.target_var = tk.StringVar()
        ttk.Entry(placement_frame, textvariable=self.target_var).grid(row=2, column=0, columnspan=2, sticky="ew")
        ttk.Label(placement_frame, text="e.g. 1234.5, 6789.0, 0.0  (leave blank to use origin/raw coordinates)",
                  style="Muted.TLabel").grid(row=3, column=0, columnspan=2, sticky="w", pady=(4, 0))

        skirt_frame = ttk.LabelFrame(body, text="Skirt height (meters)", padding=(10, 6))
        skirt_frame.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        skirt_frame.columnconfigure(1, weight=1)
        self.skirt_var = tk.StringVar(value="1.0")
        ttk.Entry(skirt_frame, textvariable=self.skirt_var).grid(row=0, column=0, columnspan=2, sticky="ew")
        ttk.Label(skirt_frame, text="Height of the mesh skirt below the base level. Must be >= 0.",
                  style="Muted.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(4, 0))

    def _browse(self) -> None:
        path = filedialog.askopenfilename(
            title="Select a 3D DXF contours only file",
            filetypes=[("DXF files", "*.dxf"), ("All files", "*.*")],
            initialdir=os.path.dirname(self.dxf_path) if self.dxf_path else None,
            parent=self.window.root)
        if path:
            self.file_var.set(path)

    def _parse_target(self):
        text = self.target_var.get().strip()
        if not text:
            return None
        parts = [float(x.strip()) for x in text.split(',')]
        if len(parts) == 2:
            parts.append(0.0)
        return (parts[0], parts[1], parts[2])

    def _parse_skirt(self) -> float:
        text = self.skirt_var.get().strip()
        if not text:
            return DEFAULT_SKIRT
        try:
            return max(0.0, float(text))
        except ValueError:
            return DEFAULT_SKIRT
    def create_mesh(self) -> None:
        dxf_path = self.file_var.get().strip()
        if not dxf_path or not os.path.isfile(dxf_path):
            self.window.set_status("Pick a DXF file first.", "error")
            return
        self.dxf_path = dxf_path
        origin = bool(self.origin_var.get())
        target = self._parse_target()
        units = self.units_var.get().strip().lower() or "auto"
        skirt = self._parse_skirt()

        with self.window.busy("Reading and building the mesh..."):
            rings = parse_dxf(dxf_path)
            rings = [r for r in (dedup_ring(r) for r in rings) if len(r) >= 3]
            if not rings:
                self.window.set_status("No valid contour rings found in the DXF.", "error")
                return

            if units in UNIT_NAMES:
                unit_code = UNIT_NAMES[units]
                self.window.set_status(f"Units: {units} (from options)", "info")
            else:
                unit_code = read_dxf_units(dxf_path)
                self.window.set_status(f"Units: {UNIT_NAMES_REV.get(unit_code, 'unknown')} "
                                       f"(from DXF $INSUNITS={unit_code})", "info")
            scale = dxf_units_to_meters(unit_code)

            all_pts = [p for r in rings for p in r]
            min_x = min(p[0] for p in all_pts)
            min_y = min(p[1] for p in all_pts)

            if target:
                tx, ty, tz = target
                cx = min_x * scale - tx
                cy = min_y * scale - ty
                cz = -tz
            elif origin:
                cx = min_x * scale
                cy = min_y * scale
                cz = 0.0
            else:
                cx = cy = cz = 0.0

            mesh_data = rings_to_tapir_mesh(rings, offset_x=-cx, offset_y=-cy, offset_z=-cz,
                                            scale=scale, skirt_height=skirt)
            n_outer = len(mesh_data["polygonCoordinates"])
            n_sub = len(mesh_data.get("sublines", []))
            self.window.set_status(f"Building mesh: {n_outer} outer pts, {n_sub} ridge lines. "
                                   f"Creating in ArchiCAD...", "info")

            run_tapir("CreateMeshes", {"meshesData": [mesh_data]})

            self.window.set_status(
                f"Created a terrain mesh with {n_sub} contour ridge lines. "
                f"Press F (Zoom Full Drawing) to see it.", "ok")


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ContourMesh().window.run()
