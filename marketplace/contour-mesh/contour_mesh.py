# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# archicad-versions = [30]
# min-tapir-version = "1.5.0"
# changes = "model"
# ///

"""
ContourMesh

Reads a DXF file with 3D contour lines and creates a single native Archicad
Mesh from them. The outer boundary is the convex hull of all the points with
its Z interpolated to the ground, and every contour ring becomes a subline at
its own elevation, so the mesh reads as a 3D terrain with contour ridge lines.

The mesh is placed at its raw DXF coordinates by default; you can move it so
its lower left corner is at the project origin (X and Y only) or at a specific
coordinate, override the unit system the DXF coordinates are in (the header's
$INSUNITS is often wrong), and set the height of the skirt below the base level.

Original by Paul K (ArchPrime), MIT licence.
"""

import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import run_tapir, item_errors
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


def is_contour_ring(pts):
    """A genuine contour line has the SAME Z value at every vertex.

    Contours are computed at exact elevations, so all points on one contour
    share an identical Z (to floating-point precision). Other 3D geometry
    (fence tops, roof eaves, etc.) has Z that varies along the line, so it
    fails this test and is filtered out. No tolerance: a real contour is
    exactly flat in Z.
    """
    if not pts:
        return False
    z0 = pts[0][2]
    return all(p[2] == z0 for p in pts)


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
    in_polyline = False  # True while inside a POLYLINE entity (reading VERTEX points)
    def flush():
        nonlocal cur_pts, cur_closed, cur_elev, in_polyline
        if len(cur_pts) >= 3:
            # Apply elevation to all points if they're all at z=0
            if all(abs(p[2]) < 1e-10 for p in cur_pts) and abs(cur_elev) > 1e-10:
                cur_pts = [(p[0], p[1], cur_elev) for p in cur_pts]
            # Keep only genuine contour rings: Z must be constant along the
            # whole ring. Other 3D geometry (fence tops, roof eaves, ...) has
            # Z that varies along the line and is filtered out here.
            if is_contour_ring(cur_pts):
                rings.append((cur_pts[:], cur_closed))
        cur_pts = []
        cur_closed = False
        cur_elev = 0.0
        in_polyline = False
    for idx, (code, val) in enumerate(pairs):
        if code == '0':
            etype = val.strip()
            if in_polyline and etype == 'VERTEX':
                # A vertex of the open POLYLINE: keep collecting its points.
                cur_entity = etype
                continue
            # SEQEND closes the open POLYLINE; any other entity ends the
            # previous one. Either way the collected ring is finished here.
            flush()
            if etype == 'POLYLINE':
                in_polyline = True
            cur_entity = etype
        elif in_polyline:
            # Inside a POLYLINE entity: read its own codes or VERTEX points
            if cur_entity == 'VERTEX':
                if code == '10':
                    try:
                        cur_pts.append((float(val), 0.0, 0.0))
                    except ValueError:
                        pass
                elif code == '20':
                    try:
                        if cur_pts:
                            x, y, z = cur_pts[-1]
                            cur_pts[-1] = (x, float(val), z)
                    except ValueError:
                        pass
                elif code == '30':
                    try:
                        if cur_pts:
                            x, y, z = cur_pts[-1]
                            cur_pts[-1] = (x, y, float(val))
                    except ValueError:
                        pass
            else:
                # POLYLINE's own codes
                if code == '70':
                    try:
                        cur_closed = (int(val) & 1) != 0
                    except ValueError:
                        pass
                elif code == '30':
                    try:
                        cur_elev = float(val)
                    except ValueError:
                        pass
        elif cur_entity == 'LWPOLYLINE':
            if code == '70':
                try:
                    cur_closed = (int(val) & 1) != 0
                except ValueError:
                    pass
            elif code == '38':
                # LWPOLYLINE elevation (whole-polyline Z fallback)
                try:
                    cur_elev = float(val)
                except ValueError:
                    pass
            elif code == '10':
                try:
                    cur_pts.append((float(val), 0.0, 0.0))
                except ValueError:
                    pass
            elif code == '20':
                try:
                    if cur_pts:
                        x, y, z = cur_pts[-1]
                        cur_pts[-1] = (x, float(val), z)
                except ValueError:
                    pass
            elif code == '23':
                # 3D LWPOLYLINE stores per-point Z in code 23 (not 30)
                try:
                    if cur_pts:
                        x, y, z = cur_pts[-1]
                        cur_pts[-1] = (x, y, float(val))
                except ValueError:
                    pass
        elif cur_entity == 'LINE':
            if code == '10':
                try:
                    cur_pts.append((float(val), 0.0, 0.0))
                except ValueError:
                    pass
            elif code == '20':
                try:
                    if cur_pts:
                        x, y, z = cur_pts[-1]
                        cur_pts[-1] = (x, float(val), z)
                except ValueError:
                    pass
            elif code == '30':
                try:
                    if cur_pts:
                        x, y, z = cur_pts[-1]
                        cur_pts[-1] = (x, y, float(val))
                except ValueError:
                    pass
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


def _nearest_z(point, all_pts):
    """Find the Z of the nearest contour point to (x, y).

    Plain-Python linear scan: the hull has a small number of vertices, so a
    scan over all contour points is cheap and avoids a numpy/scipy dependency.
    """
    if not all_pts:
        return 0.0
    best = None
    best_d2 = float('inf')
    for p in all_pts:
        dx = p[0] - point[0]
        dy = p[1] - point[1]
        d2 = dx * dx + dy * dy
        if d2 < best_d2:
            best_d2 = d2
            best = p
    return best[2] if best is not None else 0.0


def _hull_outer_with_z(rings, scale, offset_x, offset_y, offset_z):
    """Convex-hull outer boundary with Z interpolated from the nearest contour.

    The convex hull of all points is always a simple (non-self-intersecting)
    polygon, which is the one hard requirement Tapir imposes on the outer ring.
    Each hull vertex is given the Z of the nearest contour point so the base
    of the terrain tracks the ground surface.

    rings is a list of (pts, closed) tuples.
    """
    all_pts = [p for pts, _closed in rings for p in pts]
    hull = convex_hull([(p[0], p[1]) for p in all_pts])
    outer = []
    for hx, hy in hull:
        z = _nearest_z((hx, hy), all_pts)
        outer.append({"x": hx*scale + offset_x, "y": hy*scale + offset_y, "z": z*scale + offset_z})
    return outer
def rings_to_tapir_mesh(rings, offset_x=0.0, offset_y=0.0, offset_z=0.0, scale=1.0, min_area=1.0, skirt_height=DEFAULT_SKIRT):
    """Build a SINGLE triangulated terrain mesh from contour rings.

    rings is a list of (pts, closed) tuples from parse_dxf.

    Approach (verified against Archicad 30 / Tapir source):
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
    # Dedup and filter. min_area is in SQUARE METERS, so compare the
    # ring's area against min_area/scale^2 in raw DXF units. This keeps the
    # filter consistent regardless of the unit system (a 1 m^2 ring is a
    # 1 m^2 ring whether the DXF is in meters, inches, or feet).
    # Apply the area filter only to closed rings; for open contours, filter
    # on length instead so that nearly-straight edge contours are not dropped.
    # The length limit is the side of a min_area square (1 m for 1 m^2),
    # converted to raw DXF units the same way (divided by scale, not scale^2).
    min_area_raw = min_area / (scale * scale) if scale else min_area
    min_length_raw = (min_area ** 0.5) / scale if scale else min_area ** 0.5
    filtered = []
    for pts, closed in rings:
        pts = dedup_ring(pts)
        if len(pts) < 3:
            continue
        if closed:
            if ring_area(pts) < min_area_raw:
                continue
        else:
            # Open contour: filter by length (drop tiny fragments)
            length = sum(
                ((pts[i+1][0]-pts[i][0])**2 + (pts[i+1][1]-pts[i][1])**2) ** 0.5
                for i in range(len(pts)-1)
            )
            if length < min_length_raw:
                continue
        filtered.append((pts, closed))
    if not filtered:
        raise ValueError("All rings degenerate after dedup")

    outer = _hull_outer_with_z(filtered, scale, offset_x, offset_y, offset_z)
    if len(outer) < 3:
        raise ValueError(f"Convex hull has {len(outer)} pts")

    def pt(p):
        return {"x": p[0]*scale + offset_x, "y": p[1]*scale + offset_y, "z": p[2]*scale + offset_z}

    # CRITICAL: Tapir expects each subline to be an OBJECT with a "coordinates"
    # key, NOT a bare array. The 3D coordinate (including Z) of each point is
    # read via Get3DCoordinateFromObjectState, so the subline's Z values create
    # real 3D relief (the contour ridge lines). Passing bare arrays makes Tapir
    # ignore the sublines entirely (nSubLines stays 0).
    # dedup_ring dropped the closing point of closed contours; repeat the first
    # point at the end so the ridge line also runs from the last point back.
    sublines = [{"coordinates": [pt(p) for p in (pts + [pts[0]] if closed else pts)]}
                for pts, closed in filtered]
    mesh = {"polygonCoordinates": outer}
    if sublines:
        mesh["sublines"] = sublines
    # showLines makes Archicad render the mesh edges (the contour ridge lines)
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
        min_z_m = min(p[2] for pts, _c in filtered for p in pts) * scale + offset_z
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
        ttk.Checkbutton(placement_frame, text="Move the mesh's lower left corner to the project origin "
                                              "(X and Y only, heights stay)",
                        variable=self.origin_var).grid(row=0, column=0, columnspan=2, sticky="w")
        ttk.Label(placement_frame, text="Target location in meters (optional, overrides origin):",
                  style="Muted.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.target_var = tk.StringVar()
        ttk.Entry(placement_frame, textvariable=self.target_var).grid(row=2, column=0, columnspan=2, sticky="ew")
        ttk.Label(placement_frame, text="e.g. 1234.5, 6789.0, 0.0  (leave blank to use origin/raw coordinates)",
                  style="Muted.TLabel").grid(row=3, column=0, columnspan=2, sticky="w", pady=(4, 0))

        skirt_frame = ttk.LabelFrame(body, text="Thickness below lowest contour (meters)", padding=(10, 6))
        skirt_frame.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        skirt_frame.columnconfigure(1, weight=1)
        self.skirt_var = tk.StringVar(value="1.0")
        ttk.Entry(skirt_frame, textvariable=self.skirt_var).grid(row=0, column=0, columnspan=2, sticky="ew")
        ttk.Label(skirt_frame, text="Thickness of the mesh below the lowest contour. Must be >= 0.",
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
        """ The target X, Y(, Z) in meters, None when blank; raises ValueError with a clear message. """
        text = self.target_var.get().strip()
        if not text:
            return None
        try:
            parts = [float(x.strip()) for x in text.split(',')]
        except ValueError:
            parts = []
        if len(parts) == 2:
            parts.append(0.0)
        if len(parts) != 3:
            raise ValueError(f"Target location '{text}' is not 'X, Y' or 'X, Y, Z' in meters, "
                             "e.g. 1234.5, 6789.0, 0.0.")
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
        try:
            target = self._parse_target()
        except ValueError as e:
            self.window.set_status(str(e), "error")
            return
        units = self.units_var.get().strip().lower() or "auto"
        skirt = self._parse_skirt()

        with self.window.busy("Reading and building the mesh..."):
            raw_rings = parse_dxf(dxf_path)
            # Dedup only - rings_to_tapir_mesh handles the area/length filtering
            # with the correct unit conversion (min_area / scale^2).
            rings = [(dedup_ring(pts), closed) for pts, closed in raw_rings if len(dedup_ring(pts)) >= 3]
            if not rings:
                self.window.set_status("No valid contour rings found in the DXF. Nothing was created.", "error")
                return

            if units in UNIT_NAMES:
                unit_code = UNIT_NAMES[units]
                unit_text = f"{units} (from options)"
            else:
                unit_code = read_dxf_units(dxf_path)
                unit_text = (f"{UNIT_NAMES_REV.get(unit_code, 'unknown')} "
                             f"(from DXF $INSUNITS={unit_code})")
            self.window.set_status(f"Units: {unit_text}", "info")
            scale = dxf_units_to_meters(unit_code)

            all_pts = [p for pts, _closed in rings for p in pts]
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
                # Preserve absolute Z: a DXF Z=0 contour stays at Archicad Z=0.
                # Only X/Y are shifted so the bbox min corner lands at the origin.
                cz = 0.0
            else:
                cx = cy = cz = 0.0

            try:
                mesh_data = rings_to_tapir_mesh(rings, offset_x=-cx, offset_y=-cy, offset_z=-cz,
                                                scale=scale, skirt_height=skirt)
            except ValueError as e:
                self.window.set_status(f"Cannot build a mesh from this DXF ({e}). Nothing was created.", "error")
                return
            n_outer = len(mesh_data["polygonCoordinates"])
            n_sub = len(mesh_data.get("sublines", []))

        # Say what is about to change before changing it.
        if not self.window.ask_ok_cancel(
                SCRIPT_NAME,
                f"Create one terrain mesh with {n_sub} contour lines "
                f"({n_outer} outline points, units: {unit_text}) in the current project?"):
            self.window.set_status("Cancelled. Nothing was created.", "info")
            return

        with self.window.busy("Creating the mesh in Archicad..."):
            response = run_tapir("CreateMeshes", {"meshesData": [mesh_data]})
            errors = item_errors(response.get("elements", []))
            if errors:
                self.window.set_status(
                    f"CreateMeshes reported {len(errors)} error(s). First: {errors[0]}",
                    "error")
            else:
                self.window.set_status(
                    f"Created a terrain mesh with {n_sub} contour ridge lines. "
                    f"Press F (Zoom Full Drawing) to see it.", "ok")


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ContourMesh().window.run()
