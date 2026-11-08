# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Room Finishes from Zones

Adds the finishes of each selected zone:
- the floor finish, a slab with the zone's polygon, has its top face at the
  zone's base (the finish floor level) and grows downwards by its thickness;
- the ceiling, a slab with the zone's polygon, has its bottom face at the room
  height (the zone's base plus the zone's height, or a height typed in the
  window) and grows upwards;
- the wall finishes (Archicad 28 and later): a wall along each part of a zone
  edge that has a wall behind it (from GetZoneBoundaries), with its reference
  line on the room edge and its thickness growing into the room, from the floor
  finish's top to the ceiling's bottom. Edges without a wall get none; the
  pieces of two edges meet at the corner.

Each kind is made from a favorite of its tool or, without one, from a layer, a
building material, composite (or, for walls, complex profile) and a thickness.
Every finish gets the Element ID FIN-<zone number>-floor, -ceiling or
-wall-<n>, so a second run on the same zones replaces them instead of adding
duplicates.

Preview lists the zones with their levels, draws the selected one and sums up
each story. It also warns when a structural slab overlaps a finish, in plan and
in height (for a floor finish: the structural slab's top must be at the zone's
base minus the finish's thickness). Create asks first.
"""

import math
import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Room Finishes from Zones"
EPSILON = 1e-6
LENGTH_TOLERANCE = 0.001
BOUNDARY_TOLERANCE = 0.02
MIN_WALL_PIECE = 0.05
TAG_PREFIX = "FIN"
WALL_ID_PATTERN = re.compile(r"^FIN-(.+)-wall-\d+$")
KINDS = [("floor", "Floor finish"), ("ceiling", "Ceiling"), ("wall", "Wall finish")]
SLAB_KINDS = ("floor", "ceiling")
TOOLS = {"floor": "Slab", "ceiling": "Slab", "wall": "Wall"}
PLURALS = {"floor": "floor finishes", "ceiling": "ceilings", "wall": "wall finishes"}
NO_FAVORITE = "(no favorite)"
NO_LAYER = "(the favorite's or the tool's layer)"
DEFAULT_MATERIAL = "(the tool's building material)"
BASIC_PREFIX = "Building material: "
COMPOSITE_PREFIX = "Composite: "
PROFILE_PREFIX = "Complex profile: "
DEFAULT_THICKNESS = {"floor": "0.05", "ceiling": "0.02", "wall": "0.015"}
OVERLAP_GRID = 16
DIMMED_COLOR = [200, 200, 200, 160]
ZONE_COLOR = [27, 127, 59, 255]
WARNING_COLOR = [230, 140, 0, 255]
CREATED_COLOR = [0, 130, 200, 255]


# ---- geometry (no Archicad calls) ----

def _xy(point: dict) -> tuple[float, float]:
    return float(point["x"]), float(point["y"])


def _same(a, b) -> bool:
    return abs(a[0] - b[0]) < EPSILON and abs(a[1] - b[1]) < EPSILON


def read_contour(points: list[dict] | None, arcs: list[dict] | None) -> dict:
    """ {"points": [(x, y)], "curved": {edge index}}; the repeated closing point is left out, so
    edge i runs from point i to point i + 1 (the last one back to the first). """
    pts = [_xy(p) for p in points or []]
    if len(pts) > 1 and _same(pts[0], pts[-1]):
        pts.pop()
    curved = {int(a.get("begIndex", -1)) % max(1, len(pts)) for a in arcs or []
              if abs(float(a.get("arcAngle", 0.0))) > EPSILON}
    return {"points": pts, "curved": curved}


def edges(points):
    for i, a in enumerate(points):
        yield i, a, points[(i + 1) % len(points)]


def signed_area(points) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for _, a, b in edges(points)) / 2.0 if len(points) > 2 else 0.0


def net_area(outline: dict, holes: list[dict]) -> float:
    """ The area of the outline less its holes, along straight edges (an arc counts as its chord). """
    return abs(signed_area(outline["points"])) - sum(abs(signed_area(h["points"])) for h in holes)


def inside(p, points) -> bool:
    result = False
    for _, a, b in edges(points):
        if (a[1] > p[1]) != (b[1] > p[1]):
            x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
            if p[0] < x:
                result = not result
    return result


def distance_to_segment(p, a, b) -> float:
    dx, dy = b[0] - a[0], b[1] - a[1]
    length2 = dx * dx + dy * dy
    if length2 < EPSILON * EPSILON:
        return math.hypot(p[0] - a[0], p[1] - a[1])
    t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length2))
    return math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy)


def strictly_inside(p, points, margin: float = LENGTH_TOLERANCE) -> bool:
    """ Inside the polygon and farther than margin from its edges. """
    return inside(p, points) and all(distance_to_segment(p, a, b) > margin for _, a, b in edges(points))


def bounding_box(points) -> tuple[float, float, float, float]:
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return min(xs), min(ys), max(xs), max(ys)


def polygons_overlap(a, b, grid: int = OVERLAP_GRID) -> bool:
    """ True when the two polygons share some area, not only an edge or a corner. Points of a grid
    over the overlap of their bounding boxes are tested, so a sliver narrower than a grid cell
    can be missed; that is enough for a warning. """
    if len(a) < 3 or len(b) < 3:
        return False
    ax0, ay0, ax1, ay1 = bounding_box(a)
    bx0, by0, bx1, by1 = bounding_box(b)
    x0, y0, x1, y1 = max(ax0, bx0), max(ay0, by0), min(ax1, bx1), min(ay1, by1)
    if x1 - x0 <= LENGTH_TOLERANCE or y1 - y0 <= LENGTH_TOLERANCE:
        return False
    for i in range(grid):
        for j in range(grid):
            p = (x0 + (i + 0.5) * (x1 - x0) / grid, y0 + (j + 0.5) * (y1 - y0) / grid)
            if strictly_inside(p, a) and strictly_inside(p, b):
                return True
    return False


def finish_levels(base_z: float, room_height: float, thickness: dict) -> dict:
    """ The bottom and top of each finish, as absolute heights. The floor finish's top is at the
    zone's base and the ceiling's bottom at the room height; the other face is unknown (None)
    while the thickness is (a favorite or a composite gives it only once the slab exists). """
    floor_thickness, ceiling_thickness = thickness.get("floor"), thickness.get("ceiling")
    ceiling_bottom = base_z + room_height
    return {
        "floor": (None if floor_thickness is None else base_z - floor_thickness, base_z),
        "ceiling": (ceiling_bottom, None if ceiling_thickness is None else ceiling_bottom + ceiling_thickness),
    }


def slab_bottom_top(details: dict) -> tuple[float, float] | None:
    """ The bottom and top of a slab from its GetDetailsOfElements details: zCoordinate is the
    height of its reference plane and offsetFromTop the distance of that plane below the top. """
    try:
        top = float(details["zCoordinate"]) + float(details.get("offsetFromTop", 0.0))
        return top - float(details["thickness"]), top
    except (KeyError, TypeError, ValueError):
        return None


def heights_overlap(a: tuple[float, float], b: tuple[float, float]) -> bool:
    """ True when the two height ranges (bottom, top) share more than LENGTH_TOLERANCE. """
    return min(a[1], b[1]) - max(a[0], b[0]) > LENGTH_TOLERANCE


def clashing_slabs(polygon, finish_range: tuple[float | None, float | None], slabs: list[dict]) -> list[dict]:
    """ The slabs that overlap the finish in plan and in height. A finish whose thickness is not
    known yet (bottom or top None) is taken as a sheet at its known face. """
    bottom, top = finish_range
    if bottom is None:
        bottom = top - LENGTH_TOLERANCE * 2
    if top is None:
        top = bottom + LENGTH_TOLERANCE * 2
    return [s for s in slabs if heights_overlap((bottom, top), s["range"]) and polygons_overlap(polygon, s["points"])]


def _plan_xyz(point: dict) -> tuple[float, float, float]:
    return float(point["x"]), float(point["y"]), float(point.get("z", 0.0))


def boundary_segment(polygon: list[dict] | None,
                     tolerance: float = BOUNDARY_TOLERANCE) -> tuple[tuple, tuple] | None:
    """ The plan trace of a vertical boundary face of a zone (a GetZoneBoundaries polygonOutline):
    the two plan points farthest apart, when every point lies on the line between them. None for a
    face that is not vertical (the floor, the ceiling, a sloped face) or too small to matter. """
    points = [_plan_xyz(p) for p in polygon or []]
    if len(points) < 3:
        return None
    zs = [p[2] for p in points]
    if max(zs) - min(zs) < tolerance:
        return None
    plan = [(p[0], p[1]) for p in points]
    best, a, b = -1.0, None, None
    for i, p in enumerate(plan):
        for q in plan[i + 1:]:
            d = math.hypot(q[0] - p[0], q[1] - p[1])
            if d > best:
                best, a, b = d, p, q
    if best < tolerance:
        return None
    if any(distance_to_segment(p, a, b) > tolerance for p in plan):
        return None
    return a, b


def covered_intervals(a, b, segments, tolerance: float = BOUNDARY_TOLERANCE) -> list[tuple[float, float]]:
    """ The parts of the edge a-b, as (from, to) distances from a, that the segments lie on: a
    segment counts when both its ends are within tolerance of the edge's line. Overlapping or
    touching parts are merged. """
    length = math.hypot(b[0] - a[0], b[1] - a[1])
    if length < EPSILON:
        return []
    ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
    parts = []
    for p, q in segments:
        ends = []
        for point in (p, q):
            dx, dy = point[0] - a[0], point[1] - a[1]
            if abs(dx * uy - dy * ux) > tolerance:
                break
            ends.append(dx * ux + dy * uy)
        if len(ends) < 2:
            continue
        t0, t1 = max(0.0, min(ends)), min(length, max(ends))
        if t1 - t0 > tolerance:
            parts.append((t0, t1))
    merged: list[list[float]] = []
    for t0, t1 in sorted(parts):
        if merged and t0 <= merged[-1][1] + tolerance:
            merged[-1][1] = max(merged[-1][1], t1)
        else:
            merged.append([t0, t1])
    # A part that reaches an end of the edge within tolerance ends exactly there, so the finishes of
    # two edges meet at the corner.
    result = []
    for t0, t1 in merged:
        result.append((0.0 if t0 <= tolerance else t0, length if t1 >= length - tolerance else t1))
    return result


def finish_pieces(outline: dict, holes: list[dict], segments, min_length: float = MIN_WALL_PIECE) -> list[dict]:
    """ The wall finishes of a zone: one piece per part of a straight edge (of the outline or of a
    hole) that has a wall behind it. A piece's beg and end are on the room edge and are ordered so
    that the room is on its right: an unflipped wall with its reference line on the Outside face
    grows to the right of beg -> end, so its thickness goes into the room. Curved edges are left
    out. """
    pieces = []
    for contour, is_hole in [(outline, False)] + [(h, True) for h in holes]:
        points = contour["points"]
        if len(points) < 3:
            continue
        # The room is left of the edges of a counter-clockwise outline and right of those of a
        # counter-clockwise hole.
        room_on_left = (signed_area(points) > 0) != is_hole
        for index, a, b in edges(points):
            if index in contour["curved"]:
                continue
            length = math.hypot(b[0] - a[0], b[1] - a[1])
            for t0, t1 in covered_intervals(a, b, segments):
                if t1 - t0 < min_length:
                    continue
                p = (a[0] + (b[0] - a[0]) * t0 / length, a[1] + (b[1] - a[1]) * t0 / length)
                q = (a[0] + (b[0] - a[0]) * t1 / length, a[1] + (b[1] - a[1]) * t1 / length)
                if t0 == 0.0:
                    p = a
                if t1 == length:
                    q = b
                beg, end = (q, p) if room_on_left else (p, q)
                pieces.append({"edge": index, "hole": is_hole, "beg": beg, "end": end, "length": t1 - t0})
    return pieces


def wall_finish_id(tag: str, number: int) -> str:
    return f"{TAG_PREFIX}-{tag}-wall-{number}"


def wall_finish_tag(element_id: str) -> str | None:
    """ The zone tag of a wall finish's Element ID (FIN-<tag>-wall-<n>), or None. """
    match = WALL_ID_PATTERN.match(element_id or "")
    return match.group(1) if match else None


def wall_data(zone: dict, piece: dict, number: int, settings: dict) -> dict:
    """ The CreateWalls item of one wall finish: its reference line on the room edge, unflipped
    with the reference line on its Outside face so that it grows into the room, from the zone's
    base (the floor finish's top) up to the room height (the ceiling's bottom). """
    kind_settings = settings["wall"]
    item = {
        "id": wall_finish_id(zone["tag"], number),
        "floorIndex": zone["floorIndex"],
        "begCoordinate": {"x": piece["beg"][0], "y": piece["beg"][1]},
        "endCoordinate": {"x": piece["end"][0], "y": piece["end"][1]},
        "zCoordinate": zone["bottomOffset"],
        "height": zone["roomHeight"],
        "referenceLineLocation": "Outside",
        "offset": 0.0,
        "flipped": False,
    }
    if kind_settings.get("layerIndex") is not None:
        item["layerIndex"] = kind_settings["layerIndex"]
    if kind_settings.get("favorite"):
        item["favoriteName"] = kind_settings["favorite"]
    elif kind_settings.get("compositeId"):
        item["structureType"] = "Composite"
        item["compositeId"] = kind_settings["compositeId"]
    elif kind_settings.get("profileId"):
        item["structureType"] = "Profile"
        item["profileId"] = kind_settings["profileId"]
    else:
        item["structureType"] = "Basic"
        item["thickness"] = kind_settings["thickness"]
        if kind_settings.get("buildingMaterialId"):
            item["buildingMaterialId"] = kind_settings["buildingMaterialId"]
    return item


def zone_tag(number: str, guid: str) -> str:
    """ The zone's part of the Element ID: its number with spaces made underscores, or the start
    of its guid when it has no number. """
    text = re.sub(r"\s+", "_", (number or "").strip())
    return text or guid.replace("-", "")[:8].upper()


def finish_id(tag: str, kind: str) -> str:
    return f"{TAG_PREFIX}-{tag}-{kind}"


def parse_length(text: str) -> float | None:
    try:
        return float(text.strip().replace(",", "."))
    except ValueError:
        return None


def slab_data(zone: dict, kind: str, settings: dict) -> dict:
    """ The CreateSlabs item of one finish of a zone. """
    kind_settings = settings[kind]
    levels = zone["levels"][kind]
    item = {
        "id": finish_id(zone["tag"], kind),
        "floorIndex": zone["floorIndex"],
        "polygonCoordinates": zone["polygonCoordinates"],
        "referencePlaneLocation": "Top" if kind == "floor" else "Bottom",
        "level": levels[1] if kind == "floor" else levels[0],
    }
    if zone["polygonArcs"]:
        item["polygonArcs"] = zone["polygonArcs"]
    if zone["holeData"]:
        item["holes"] = zone["holeData"]
    if kind_settings.get("layerIndex") is not None:
        item["layerIndex"] = kind_settings["layerIndex"]
    if kind_settings.get("favorite"):
        item["favoriteName"] = kind_settings["favorite"]
    elif kind_settings.get("compositeId"):
        item["structureType"] = "Composite"
        item["compositeId"] = kind_settings["compositeId"]
    else:
        item["structureType"] = "Basic"
        item["thickness"] = kind_settings["thickness"]
        if kind_settings.get("buildingMaterialId"):
            item["buildingMaterialId"] = kind_settings["buildingMaterialId"]
    return item


def known_thickness(kind_settings: dict) -> float | None:
    """ The finish's thickness when the window gives it: a Basic slab without a favorite. """
    if (not kind_settings.get("enabled") or kind_settings.get("favorite") or kind_settings.get("compositeId")
            or kind_settings.get("profileId")):
        return None
    return kind_settings.get("thickness")


def story_summary(zones: list[dict]) -> list[dict]:
    """ Per story, in story order: the zones, the finishes of each kind to create ("planned"; for
    walls the number of pieces), the zones' area and the length of the wall finishes. """
    rows: dict[int, dict] = {}
    for zone in zones:
        row = rows.setdefault(zone["floorIndex"], {"floorIndex": zone["floorIndex"], "story": zone["story"],
                                                  "zones": 0, "floor": 0, "ceiling": 0, "wall": 0, "area": 0.0,
                                                  "wallLength": 0.0, "clashes": 0})
        row["zones"] += 1
        for kind in zone["planned"]:
            if kind == "wall":
                pieces = zone.get("wallPieces", [])
                row["wall"] += len(pieces)
                row["wallLength"] += sum(p["length"] for p in pieces)
            else:
                row[kind] += 1
        row["area"] += zone["area"]
        row["clashes"] += len(zone["clashes"])
    return [rows[k] for k in sorted(rows)]


# ---- window ----

class RoomFinishesFromZones:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Adds the finishes of each selected zone: a floor finish and a ceiling, as slabs with the zone's polygon "
            "(the floor finish's top at the zone's base, the ceiling's bottom at the room height), and a wall finish "
            "along every edge with a wall behind it, growing into the room. Press Preview to check them, then Create.",
            min_width=1080, min_height=800)
        self.favorites: dict[str, list[str]] = {}
        self.layers: dict[str, int] = {}
        self.structures: dict[str, dict[str, dict]] = {}
        self.kind_vars: dict[str, dict[str, tk.Variable]] = {}
        self.kind_widgets: dict[str, dict[str, ttk.Widget]] = {}
        self.preview: dict | None = None
        self.created: list[dict] = []
        self._build_body()
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Create", self.create)
        self.window.add_button("Preview", self.make_preview, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        kinds = ttk.Frame(body)
        kinds.grid(row=0, column=0, sticky="ew")
        for column, (kind, label) in enumerate(KINDS):
            kinds.columnconfigure(column, weight=1)
            frame = ttk.LabelFrame(kinds, text=label, padding=(10, 6))
            frame.grid(row=0, column=column, sticky="nsew", padx=(0 if column == 0 else 8, 0))
            frame.columnconfigure(1, weight=1)
            vars_ = {
                "enabled": tk.BooleanVar(value=True),
                "favorite": tk.StringVar(value=NO_FAVORITE),
                "layer": tk.StringVar(value=NO_LAYER),
                "structure": tk.StringVar(value=DEFAULT_MATERIAL),
                "thickness": tk.StringVar(value=DEFAULT_THICKNESS[kind]),
            }
            self.kind_vars[kind] = vars_
            ttk.Checkbutton(frame, text=f"Create the {PLURALS[kind]}", variable=vars_["enabled"]).grid(
                row=0, column=0, columnspan=2, sticky="w")
            widgets = {}
            for row, (key, text) in enumerate([("favorite", f"{TOOLS[kind]} favorite:"), ("layer", "Layer:"),
                                               ("structure", "Structure:")], start=1):
                ttk.Label(frame, text=text).grid(row=row, column=0, sticky="w", pady=(4, 0))
                box = ttk.Combobox(frame, textvariable=vars_[key], state="readonly", width=26)
                box.grid(row=row, column=1, sticky="ew", padx=(6, 0), pady=(4, 0))
                widgets[key] = box
            ttk.Label(frame, text="Thickness (m):").grid(row=4, column=0, sticky="w", pady=(4, 0))
            widgets["thickness"] = ttk.Entry(frame, textvariable=vars_["thickness"], width=8)
            widgets["thickness"].grid(row=4, column=1, sticky="w", padx=(6, 0), pady=(4, 0))
            gives_own = "a composite or a complex profile" if kind == "wall" else "a composite"
            ttk.Label(frame, text=f"Structure and thickness are used without a favorite;\n"
                                  f"{gives_own} gives its own thickness.",
                      style="Muted.TLabel").grid(row=5, column=0, columnspan=2, sticky="w", pady=(4, 0))
            self.kind_widgets[kind] = widgets
            vars_["favorite"].trace_add("write", lambda *_a, k=kind: self._update_kind_state(k))
            vars_["structure"].trace_add("write", lambda *_a, k=kind: self._update_kind_state(k))

        options = ttk.LabelFrame(body, text="Room height and options", padding=(10, 6))
        options.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.height_source = tk.StringVar(value="zone")
        ttk.Radiobutton(options, text="The zone's height", value="zone", variable=self.height_source).grid(
            row=0, column=0, sticky="w")
        ttk.Radiobutton(options, text="This height (m):", value="typed", variable=self.height_source).grid(
            row=0, column=1, sticky="w", padx=(16, 0))
        self.typed_height = tk.StringVar(value="2.60")
        ttk.Entry(options, textvariable=self.typed_height, width=8).grid(row=0, column=2, sticky="w", padx=(6, 0))
        self.replace = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Replace the finishes an earlier run made for these zones (found by their Element ID)",
                        variable=self.replace).grid(row=1, column=0, columnspan=3, sticky="w", pady=(6, 0))
        self.check_clashes = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Warn about structural slabs that overlap a finish (nothing is moved)",
                        variable=self.check_clashes).grid(row=2, column=0, columnspan=3, sticky="w", pady=(2, 0))

        self.summary_tree = make_tree(body, [("story", "Story", 200, "w"), ("zones", "Zones", 60, "e"),
                                             ("floor", "Floor finishes", 100, "e"), ("ceiling", "Ceilings", 80, "e"),
                                             ("wall", "Wall finishes", 90, "e"),
                                             ("wallLength", "Wall finish length (m)", 140, "e"),
                                             ("area", "Area (m²)", 90, "e"), ("clashes", "Clashes", 70, "e")], height=4)
        self.summary_tree.container.grid(row=2, column=0, sticky="ew", pady=(8, 0))

        preview = ttk.Frame(body, padding=(0, 8, 0, 0))
        preview.grid(row=3, column=0, sticky="nsew")
        preview.columnconfigure(0, weight=3)
        preview.columnconfigure(1, weight=2)
        preview.rowconfigure(1, weight=1)
        self.summary_label = ttk.Label(preview, text="Select zones in Archicad and press Preview.")
        self.summary_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 4))
        self.zone_tree = make_tree(preview, [("zone", "Zone", 170, "w"), ("story", "Story", 90, "w"),
                                             ("floor", "Floor finish top", 100, "e"),
                                             ("ceiling", "Ceiling bottom", 100, "e"),
                                             ("wall", "Wall finishes", 90, "e"), ("note", "Note", 260, "w")],
                                   height=8)
        self.zone_tree.configure(selectmode="browse")
        self.zone_tree.container.grid(row=1, column=0, sticky="nsew")
        self.zone_tree.bind("<<TreeviewSelect>>", lambda _e: self._draw())
        self.zone_tree.bind("<Double-1>", lambda _e: self.window.guarded(self._select_zone)())
        self.canvas = tk.Canvas(preview, background="white", highlightthickness=1, highlightbackground="#bbbbbb",
                                width=300, height=240)
        self.canvas.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
        self.canvas.bind("<Configure>", lambda _e: self._draw())

    def _update_kind_state(self, kind: str) -> None:
        vars_, widgets = self.kind_vars[kind], self.kind_widgets[kind]
        has_favorite = vars_["favorite"].get() != NO_FAVORITE
        widgets["structure"].configure(state="disabled" if has_favorite else "readonly")
        gives_thickness = vars_["structure"].get().startswith((COMPOSITE_PREFIX, PROFILE_PREFIX))
        widgets["thickness"].configure(state="disabled" if has_favorite or gives_thickness else "normal")

    # ---- start ----

    def load(self) -> None:
        with self.window.busy("Reading the Slab and Wall favorites, layers, building materials, composites and "
                              "profiles..."):
            for tool in sorted(set(TOOLS.values())):
                try:
                    self.favorites[tool] = sorted(
                        run_tapir("GetFavoritesByType", {"elementType": tool}).get("favorites", []), key=str.lower)
                except ArchicadError as e:
                    print(f"The {tool} favorites cannot be read: {e}")
                    self.favorites[tool] = []
            self.layers = self._attribute_labels("Layer")
            slab_structures = {DEFAULT_MATERIAL: {}}
            for label, attribute in self._attributes("BuildingMaterial"):
                slab_structures[BASIC_PREFIX + label] = {"buildingMaterialId": attribute}
            for label, attribute in self._attributes("Composite"):
                slab_structures[COMPOSITE_PREFIX + label] = {"compositeId": attribute}
            wall_structures = dict(slab_structures)
            try:
                for label, attribute in self._attributes("Profile"):
                    wall_structures[PROFILE_PREFIX + label] = {"profileId": attribute}
            except ArchicadError as e:
                print(f"The complex profiles cannot be read: {e}")
            self.structures = {"Slab": slab_structures, "Wall": wall_structures}
        for kind, _ in KINDS:
            tool = TOOLS[kind]
            widgets = self.kind_widgets[kind]
            widgets["favorite"].configure(values=[NO_FAVORITE] + self.favorites[tool])
            widgets["layer"].configure(values=[NO_LAYER] + list(self.layers))
            widgets["structure"].configure(values=list(self.structures[tool]))
            self._update_kind_state(kind)
        self.window.set_status(f"{len(self.favorites['Slab'])} Slab and {len(self.favorites['Wall'])} Wall "
                               "favorite(s). Select the zones in Archicad and press Preview.")

    def _attributes(self, attribute_type: str) -> list[tuple[str, dict]]:
        """ (label, attributeId) by name; a name used twice gets its index added. """
        attributes = run_tapir("GetAttributesByType", {"attributeType": attribute_type}).get("attributes", [])
        result, seen = [], set()
        for attribute in sorted(attributes, key=lambda a: str(a.get("name", "")).lower()):
            label = str(attribute.get("name", ""))
            if label in seen:
                label = f"{label} ({attribute.get('index')})"
            seen.add(label)
            result.append((label, {"attributeId": attribute["attributeId"], "index": attribute.get("index")}))
        return result

    def _attribute_labels(self, attribute_type: str) -> dict[str, int]:
        return {label: int(a["index"]) for label, a in self._attributes(attribute_type)}

    # ---- settings ----

    def _settings(self) -> dict | None:
        """ The window's settings, or None after showing what is wrong. """
        settings = {"replace": self.replace.get(), "checkClashes": self.check_clashes.get(),
                    "heightSource": self.height_source.get(), "typedHeight": None}
        if settings["heightSource"] == "typed":
            height = parse_length(self.typed_height.get())
            if height is None or height <= EPSILON:
                self.window.set_status("The room height must be a positive number of metres.", "warning")
                return None
            settings["typedHeight"] = height
        for kind, label in KINDS:
            vars_ = self.kind_vars[kind]
            kind_settings = {"enabled": bool(vars_["enabled"].get())}
            favorite = vars_["favorite"].get()
            kind_settings["favorite"] = favorite if favorite != NO_FAVORITE else None
            layer = self.layers.get(vars_["layer"].get())
            kind_settings["layerIndex"] = layer
            structure = self.structures.get(TOOLS[kind], {}).get(vars_["structure"].get(), {})
            kind_settings["buildingMaterialId"] = (structure.get("buildingMaterialId") or {}).get("attributeId")
            kind_settings["compositeId"] = (structure.get("compositeId") or {}).get("attributeId")
            kind_settings["profileId"] = (structure.get("profileId") or {}).get("attributeId")
            kind_settings["thickness"] = None
            if (kind_settings["enabled"] and not kind_settings["favorite"] and not kind_settings["compositeId"]
                    and not kind_settings["profileId"]):
                thickness = parse_length(vars_["thickness"].get())
                if thickness is None or thickness <= EPSILON:
                    self.window.set_status(f"The {label.lower()}'s thickness must be a positive number of metres.",
                                           "warning")
                    return None
                kind_settings["thickness"] = thickness
            settings[kind] = kind_settings
        if not any(settings[kind]["enabled"] for kind, _ in KINDS):
            self.window.set_status("Tick at least one kind of finish to create.", "warning")
            return None
        return settings

    @staticmethod
    def _snapshot(settings: dict) -> str:
        return repr(settings)

    # ---- preview ----

    def _read_zones(self, settings: dict) -> list[dict]:
        zone_ids = get_elements("selection", ["Zone"])
        if not zone_ids:
            return []
        details = get_details_of_elements(zone_ids, ["type", "floorIndex", "details"])
        stories = get_story_names()
        thickness = {kind: known_thickness(settings[kind]) for kind, _ in KINDS}
        zones = []
        for zone_id, detail in zip(zone_ids, details):
            d = detail.get("details") or {}
            if detail.get("type") != "Zone" or "polygonOutline" not in d:
                continue
            outline = read_contour(d["polygonOutline"], d.get("polygonArcs"))
            if len(outline["points"]) < 3:
                continue
            holes_raw = [h for h in d.get("holes") or [] if h.get("polygonOutline") or h.get("polygonCoordinates")]
            holes = [read_contour(h.get("polygonOutline") or h.get("polygonCoordinates"), h.get("polygonArcs"))
                     for h in holes_raw]
            floor = int(detail.get("floorIndex", 0))
            base_z = float(d.get("zCoordinate", 0.0))
            if settings["heightSource"] == "typed":
                room_height = settings["typedHeight"]
            else:
                room_height = float(d.get("height") or 0.0)
            number = str(d.get("numberStr", "") or "")
            guid = zone_id["elementId"]["guid"]
            notes = []
            if room_height <= EPSILON:
                notes.append("the zone has no height: no ceiling")
            if outline["curved"]:
                notes.append("curved edges")
            zones.append({
                "elementId": zone_id["elementId"],
                "label": f"{number} {d.get('name', '')}".strip() or "(no name)",
                "story": stories.get(floor, str(floor)),
                "floorIndex": floor,
                "tag": zone_tag(number, guid),
                "outline": outline,
                "holes": holes,
                "polygonCoordinates": [{"x": float(p["x"]), "y": float(p["y"])} for p in d["polygonOutline"]],
                "polygonArcs": [{"begIndex": int(a["begIndex"]), "endIndex": int(a["endIndex"]),
                                 "arcAngle": float(a["arcAngle"])} for a in d.get("polygonArcs") or []],
                "holeData": [{"polygonOutline": [{"x": float(p["x"]), "y": float(p["y"])}
                                                 for p in (h.get("polygonOutline") or h.get("polygonCoordinates"))],
                              **({"polygonArcs": [{"begIndex": int(a["begIndex"]), "endIndex": int(a["endIndex"]),
                                                   "arcAngle": float(a["arcAngle"])} for a in h["polygonArcs"]]}
                                 if h.get("polygonArcs") else {})}
                             for h in holes_raw],
                "area": net_area(outline, holes),
                "baseZ": base_z,
                "bottomOffset": float(d.get("bottomOffset", 0.0)),
                "roomHeight": room_height,
                "levels": finish_levels(base_z, room_height, thickness),
                "noCeiling": room_height <= EPSILON,
                "notes": notes,
                "clashes": [],
                "existing": set(),
                "planned": [],
                "wallPieces": [],
            })
        # Two selected zones with the same number on one story would get the same Element ID.
        seen: dict[tuple, int] = {}
        for zone in zones:
            key = (zone["floorIndex"], zone["tag"])
            seen[key] = seen.get(key, 0) + 1
        for zone in zones:
            if seen[(zone["floorIndex"], zone["tag"])] > 1:
                zone["tag"] = f"{zone['tag']}-{zone['elementId']['guid'].replace('-', '')[:8].upper()}"
                zone["notes"].append("number used twice: tagged with its guid too")
        zones.sort(key=lambda z: (z["floorIndex"], z["label"].lower()))
        return zones

    def _read_slabs(self) -> tuple[list[dict], dict[tuple, list[dict]]]:
        """ The structural slabs ({"elementId", "id", "points", "range"}) and the finishes of earlier
        runs by (floor index, Element ID). """
        slab_ids = run_tapir("GetElementsByType", {"elementType": "Slab"}).get("elements", [])
        details = get_details_of_elements(slab_ids, ["id", "floorIndex", "details"]) if slab_ids else []
        structural, finishes = [], {}
        for slab_id, detail in zip(slab_ids, details):
            element_id = str(detail.get("id", "") or "")
            if element_id.startswith(TAG_PREFIX + "-"):
                finishes.setdefault((int(detail.get("floorIndex", 0)), element_id), []).append(
                    {"elementId": slab_id["elementId"]})
                continue
            d = detail.get("details") or {}
            heights = slab_bottom_top(d)
            points = read_contour(d.get("polygonOutline"), d.get("polygonArcs"))["points"]
            if heights is None or len(points) < 3:
                continue
            structural.append({"elementId": slab_id["elementId"], "id": element_id, "points": points, "range": heights})
        return structural, finishes

    @staticmethod
    def _read_walls() -> tuple[set[str], dict[tuple, list[dict]]]:
        """ The guids of every wall, and the wall finishes of earlier runs ({"elementId", "beg",
        "end"}) by (floor index, zone tag). """
        wall_ids = run_tapir("GetElementsByType", {"elementType": "Wall"}).get("elements", [])
        details = get_details_of_elements(wall_ids, ["id", "floorIndex"]) if wall_ids else []
        earlier = [(wall_id, detail) for wall_id, detail in zip(wall_ids, details)
                   if wall_finish_tag(str(detail.get("id", "") or ""))]
        lines = get_details_of_elements([w for w, _ in earlier], ["details"]) if earlier else []
        finishes: dict[tuple, list[dict]] = {}
        for (wall_id, detail), line in zip(earlier, lines):
            d = line.get("details") or {}
            key = (int(detail.get("floorIndex", 0)), wall_finish_tag(str(detail.get("id", ""))))
            item = {"elementId": wall_id["elementId"]}
            if "begCoordinate" in d and "endCoordinate" in d:
                item["beg"], item["end"] = _xy(d["begCoordinate"]), _xy(d["endCoordinate"])
            finishes.setdefault(key, []).append(item)
        return {w["elementId"]["guid"] for w in wall_ids}, finishes

    @staticmethod
    def _read_boundaries(zones: list[dict]) -> dict[str, list[dict] | None]:
        """ The GetZoneBoundaries boundaries of each zone by its guid (None when Archicad could not
        give them). Raises ArchicadError where the command is not available (before Archicad 28). """
        result = {}
        for chunk in batched(zones, 200):
            response = run_tapir("GetZoneBoundaries", {"zones": [{"elementId": z["elementId"]} for z in chunk]})
            for zone, item in zip(chunk, response.get("zoneBoundariesOfZones", [])):
                result[zone["elementId"]["guid"]] = (item.get("zoneBoundaries")
                                                     if isinstance(item, dict) and "error" not in item else None)
        return result

    def _plan_wall_finishes(self, zones: list[dict], settings: dict, replaced: list[dict]) -> str | None:
        """ Fills each zone's wallPieces from its boundaries and adds the earlier wall finishes to
        replace to replaced. Returns why no wall finish can be made at all, or None. """
        self.window.set_status("Reading the walls and the zone boundaries...")
        wall_guids, earlier = self._read_walls()
        try:
            boundaries = self._read_boundaries(zones)
        except ArchicadError as e:
            print(f"GetZoneBoundaries: {e}")
            return "wall finishes need Archicad 28 or later (GetZoneBoundaries)"
        for zone in zones:
            if zone["roomHeight"] <= EPSILON:
                zone["notes"].append("no wall finish: the zone has no height")
                continue
            old = earlier.get((zone["floorIndex"], zone["tag"]), [])
            if old and not settings["replace"]:
                zone["existing"].add("wall")
                zone["notes"].append("already has wall finishes: kept")
                continue
            zone_boundaries = boundaries.get(zone["elementId"]["guid"])
            if zone_boundaries is None:
                zone["notes"].append("no wall finish: Archicad gave no boundaries for the zone")
                continue
            replaced += [{"elementId": w["elementId"]} for w in old]
            old_guids = {w["elementId"]["guid"] for w in old}
            connected = [str((b.get("connectedElementId") or {}).get("guid", "")) for b in zone_boundaries]
            if old_guids and any(guid in old_guids for guid in connected):
                # The zone was updated to the finishes of the earlier run, so its edges are their
                # inner faces: the new finishes take the reference lines of the old ones, which are on
                # the room edges the zone had then.
                pieces = [{"edge": None, "hole": False, "beg": w["beg"], "end": w["end"],
                           "length": math.hypot(w["end"][0] - w["beg"][0], w["end"][1] - w["beg"][1])}
                          for w in old if "beg" in w]
                zone["notes"].append(f"follows the lines of its {len(pieces)} earlier wall finish(es)")
            else:
                segments = [segment for b, guid in zip(zone_boundaries, connected)
                            if guid in wall_guids and guid not in old_guids
                            and (segment := boundary_segment(b.get("polygonOutline"))) is not None]
                pieces = finish_pieces(zone["outline"], zone["holes"], segments)
                if zone["outline"]["curved"] or any(h["curved"] for h in zone["holes"]):
                    zone["notes"].append("no wall finish on curved edges")
            if not pieces:
                zone["notes"].append("no wall behind any edge: no wall finish")
                continue
            zone["wallPieces"] = pieces
            zone["planned"].append("wall")
        return None

    def make_preview(self) -> bool:
        settings = self._settings()
        if settings is None:
            return False
        with self.window.busy("Reading the zones..."):
            zones = self._read_zones(settings)
            if not zones:
                self.window.set_status("Select some zones in Archicad first.", "warning")
                return False
            self.window.set_status("Reading the slabs...")
            structural, finishes = self._read_slabs()
            replaced = []
            for zone in zones:
                for kind, label in KINDS:
                    if kind not in SLAB_KINDS:
                        continue
                    if not settings[kind]["enabled"] or (kind == "ceiling" and zone["noCeiling"]):
                        continue
                    old = finishes.get((zone["floorIndex"], finish_id(zone["tag"], kind)), [])
                    if old and settings["replace"]:
                        replaced += old
                    elif old:
                        zone["existing"].add(kind)
                        zone["notes"].append(f"already has a {label.lower()}: kept")
                        continue
                    zone["planned"].append(kind)
                    if settings["checkClashes"]:
                        for slab in clashing_slabs(zone["outline"]["points"], zone["levels"][kind], structural):
                            zone["clashes"].append({"kind": kind, "slab": slab})
                if zone["clashes"]:
                    zone["notes"].append(f"{len(zone['clashes'])} clash(es) with structural slabs")
            no_walls = self._plan_wall_finishes(zones, settings, replaced) if settings["wall"]["enabled"] else None
        self.preview = {"snapshot": self._snapshot(settings), "settings": settings, "zones": zones,
                        "replaced": replaced, "structural": structural}
        self.created = []
        self._show_preview()
        self._highlight(zones)
        slabs = sum(1 for _, kind in self._planned() if kind in SLAB_KINDS)
        walls = sum(len(z["wallPieces"]) for z in zones if "wall" in z["planned"])
        clashes = sum(len(z["clashes"]) for z in zones)
        message = f"Preview: {slabs} finish slab(s) and {walls} wall finish(es) for {len(zones)} zone(s)"
        if replaced:
            message += f", replacing {len(replaced)} of an earlier run"
        message += "."
        if no_walls:
            message += f" No wall finishes: {no_walls}."
        if clashes:
            message += (f" {clashes} clash(es) with structural slabs: lower their top to the zone's base minus the "
                        "floor finish's thickness, or check the ceiling's height.")
        self.window.set_status(message + " Check it, then press Create.", "warning" if clashes or no_walls else "ok")
        return True

    def _planned(self) -> list[tuple[dict, str]]:
        """ (zone, kind) of every finish the preview creates. """
        if not self.preview:
            return []
        return [(z, kind) for z in self.preview["zones"] for kind in z["planned"]]

    def _show_preview(self) -> None:
        self.summary_tree.delete(*self.summary_tree.get_children())
        for index, row in enumerate(story_summary(self.preview["zones"])):
            self.summary_tree.insert("", "end", iid=str(index), values=(
                row["story"], row["zones"], row["floor"], row["ceiling"], row["wall"], f"{row['wallLength']:.2f}",
                f"{row['area']:.2f}", row["clashes"]))
        self.zone_tree.delete(*self.zone_tree.get_children())
        for index, zone in enumerate(self.preview["zones"]):
            floor = f"{zone['levels']['floor'][1]:.3f}" if "floor" in zone["planned"] else "-"
            ceiling = f"{zone['levels']['ceiling'][0]:.3f}" if "ceiling" in zone["planned"] else "-"
            walls = len(zone["wallPieces"]) if "wall" in zone["planned"] else "-"
            self.zone_tree.insert("", "end", iid=str(index),
                                  values=(zone["label"], zone["story"], floor, ceiling, walls, "; ".join(zone["notes"])))
        self.summary_label.configure(text=f"{len(self.preview['zones'])} zone(s). Levels are absolute heights in "
                                          "metres. Select a zone to draw it; double-click selects it in Archicad.")
        if self.preview["zones"]:
            self.zone_tree.selection_set("0")
        self._draw()

    def _draw(self) -> None:
        self.canvas.delete("all")
        if not self.preview or not self.preview["zones"]:
            return
        rows = self.zone_tree.selection()
        zone = self.preview["zones"][int(rows[0])] if rows else self.preview["zones"][0]
        points = zone["outline"]["points"]
        x0, y0, x1, y1 = bounding_box(points)
        width, height = max(10, self.canvas.winfo_width()), max(10, self.canvas.winfo_height())
        pad = 18
        section = 70  # the strip at the bottom with the levels
        plan_height = max(10, height - section)
        scale = min((width - 2 * pad) / max(EPSILON, x1 - x0), (plan_height - 2 * pad) / max(EPSILON, y1 - y0))
        ox = pad + ((width - 2 * pad) - (x1 - x0) * scale) / 2
        oy = pad + ((plan_height - 2 * pad) - (y1 - y0) * scale) / 2

        def to_canvas(p):
            return ox + (p[0] - x0) * scale, plan_height - (oy + (p[1] - y0) * scale)

        for clash in zone["clashes"]:
            slab_points = clash["slab"]["points"]
            self.canvas.create_polygon(*[c for p in slab_points for c in to_canvas(p)], outline="#e68c00",
                                       fill="", dash=(4, 2))
        self.canvas.create_polygon(*[c for p in points for c in to_canvas(p)], outline="#444444", fill="#eef5ee")
        for hole in zone["holes"]:
            if len(hole["points"]) > 2:
                self.canvas.create_polygon(*[c for p in hole["points"] for c in to_canvas(p)], outline="#444444",
                                           fill="white")
        if "wall" in zone["planned"]:
            # Each wall finish as a thick line, moved a little to its right (into the room).
            for piece in zone["wallPieces"]:
                (bx, by), (ex, ey) = to_canvas(piece["beg"]), to_canvas(piece["end"])
                length = math.hypot(ex - bx, ey - by)
                if length < 1:
                    continue
                # The canvas's y axis points down, so the plan's right-hand normal is (-dy, dx) there.
                nx, ny = (ey - by) / length * -3, (ex - bx) / length * 3
                self.canvas.create_line(bx + nx, by + ny, ex + nx, ey + ny, fill="#0082c8", width=4)
        lines = [f"Zone base {zone['baseZ']:.3f} m, room height {zone['roomHeight']:.3f} m"]
        if "floor" in zone["planned"]:
            bottom, top = zone["levels"]["floor"]
            lines.append(f"Floor finish: top {top:.3f}" + (f", bottom {bottom:.3f}" if bottom is not None
                                                          else ", grows down by its thickness"))
        if "ceiling" in zone["planned"]:
            bottom, top = zone["levels"]["ceiling"]
            lines.append(f"Ceiling: bottom {bottom:.3f}" + (f", top {top:.3f}" if top is not None
                                                           else ", grows up by its thickness"))
        if "wall" in zone["planned"]:
            length = sum(p["length"] for p in zone["wallPieces"])
            lines.append(f"Wall finishes (blue): {len(zone['wallPieces'])}, {length:.2f} m long, from "
                         f"{zone['baseZ']:.3f} to {zone['baseZ'] + zone['roomHeight']:.3f}")
        self.canvas.create_text(8, plan_height + 4, anchor="nw", text="\n".join(lines), fill="#333333")

    def _highlight(self, zones: list[dict]) -> None:
        if zones:
            run_tapir("HighlightElements", {
                "elements": [{"elementId": z["elementId"]} for z in zones],
                "highlightedColors": [WARNING_COLOR if z["clashes"] else ZONE_COLOR for z in zones],
                "nonHighlightedColor": DIMMED_COLOR,
            })

    def _select_zone(self) -> None:
        row = self.zone_tree.focus()
        if row and self.preview:
            zone = self.preview["zones"][int(row)]
            select_elements([{"elementId": zone["elementId"]}])
            self.window.set_status(f"Selected {zone['label']} in Archicad.", "ok")

    # ---- creating ----

    def create(self) -> None:
        settings = self._settings()
        if settings is None:
            return
        if self.preview is None or self.preview["snapshot"] != self._snapshot(settings):
            if self.make_preview():
                self.window.set_status("The preview is updated to the current settings. Check it, then press Create "
                                       "again.", "warning")
            return
        if self.created:
            self.window.set_status("These finishes are already created. Press Preview to create them again.", "warning")
            return
        slab_items = [slab_data(zone, kind, settings) for zone, kind in self._planned() if kind in SLAB_KINDS]
        wall_items = [wall_data(zone, piece, number, settings)
                      for zone, kind in self._planned() if kind == "wall"
                      for number, piece in enumerate(zone["wallPieces"], start=1)]
        total = len(slab_items) + len(wall_items)
        if not total:
            self.window.set_status("There is nothing to create.", "warning")
            return
        replaced = self.preview["replaced"]
        lines = [f"Create {len(slab_items)} finish slab(s) and {len(wall_items)} wall finish(es) for "
                 f"{len(self.preview['zones'])} zone(s)?"]
        for kind, label in KINDS:
            kind_settings = settings[kind]
            if kind_settings["enabled"]:
                source = kind_settings["favorite"] or self.kind_vars[kind]["structure"].get()
                lines.append(f"{label}: {source}")
        if replaced:
            lines.append(f"\n{len(replaced)} finish(es) of an earlier run are deleted first (a separate Undo step).")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, "\n".join(lines)):
            return

        errors, created_slabs, created_walls = [], [], []
        with self.window.busy(f"Creating {total} finish(es)..."):
            if replaced:
                self.window.set_status(f"Deleting {len(replaced)} finish(es) of an earlier run...")
                result = run_tapir("DeleteElements", {"elements": replaced})
                delete_errors = item_errors(result.get("executionResults", []))
                if delete_errors:
                    # Creating now would put a second finish next to the one that stayed.
                    self.preview = None
                    for error in delete_errors[:20]:
                        print(f"Error: {error}")
                    self.window.set_status(f"{len(delete_errors)} finish(es) of an earlier run could not be deleted "
                                           f"(the first: {delete_errors[0]}), so nothing is created. Reserve or unlock "
                                           "them, then press Preview again.", "error")
                    return
            for command, key, items, created in [("CreateSlabs", "slabsData", slab_items, created_slabs),
                                                 ("CreateWalls", "wallsData", wall_items, created_walls)]:
                for chunk in batched(items, 500):
                    result = run_tapir(command, {key: chunk}).get("elements", [])
                    created += [{"elementId": e["elementId"]} for e in result
                                if isinstance(e, dict) and "elementId" in e]
                    errors += item_errors(result)
            clashes = self._check_created(created_slabs) if created_slabs and settings["checkClashes"] else []

        created = created_slabs + created_walls
        self.created = created
        if created:
            run_tapir("HighlightElements", {
                "elements": created,
                "highlightedColors": [CREATED_COLOR] * len(created),
                "nonHighlightedColor": DIMMED_COLOR,
            })
        for error in errors[:20]:
            print(f"Error: {error}")
        for clash in clashes[:50]:
            print(f"Clash: {clash}")
        message = (f"Created {len(created_slabs)} of {len(slab_items)} finish slab(s) and {len(created_walls)} of "
                   f"{len(wall_items)} wall finish(es); they are highlighted in blue.")
        kind = "ok"
        if clashes:
            message += f" {len(clashes)} overlap(s) with structural slabs, listed in the palette's output."
            kind = "warning"
        if errors:
            message += f" {len(errors)} error(s), the first: {errors[0]}"
            kind = "error" if not created else "warning"
        self.window.set_status(message, kind)

    def _check_created(self, created: list[dict]) -> list[str]:
        """ Checks the new slabs, now with their real thickness, against the structural slabs. """
        details = get_details_of_elements(created, ["id", "details"])
        result = []
        for detail in details:
            d = detail.get("details") or {}
            heights = slab_bottom_top(d)
            points = read_contour(d.get("polygonOutline"), d.get("polygonArcs"))["points"]
            if heights is None or len(points) < 3:
                continue
            for slab in clashing_slabs(points, heights, self.preview["structural"]):
                result.append(f"{detail.get('id', '')} (from {heights[0]:.3f} to {heights[1]:.3f} m) overlaps the slab "
                              f"{slab['id'] or slab['elementId']['guid']} (from {slab['range'][0]:.3f} to "
                              f"{slab['range'][1]:.3f} m)")
        return result

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    def run(self) -> None:
        self.window.run(on_start=self.load, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    RoomFinishesFromZones().run()
