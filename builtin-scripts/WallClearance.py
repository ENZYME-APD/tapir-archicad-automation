# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "quality-assurance"
# changes = "model"
# ///

"""
Wall Clearance

Finds the doors, windows and objects whose space is too close to a wall and
moves them just enough. One rule for every kind:
- the element's space is its own floor plan outline (the convex hull of what
  Archicad draws for it: an object's symbol, rotated with it) plus a margin.
  Archicad draws doors and windows as part of their wall and gives no outline
  of their own, so a door's or window's space is its opening in the wall: its
  width (centerOffset +/- width / 2) across the wall's thickness. The swing of
  a door is not part of it;
- the obstructions are the walls, columns and curtain walls cut on its story
  (their floor plan cut fill, GetDetailsOfElements floorPlanPolygons), except
  the wall a door or window sits in and the wall an object stands against;
- the space must not come closer to an obstruction than the margin. Now in the
  list is the smallest gap (negative when they overlap), Needed is the margin.

The fix has one direction per kind: a door or window slides along its own wall
(its centerOffset, ModifyDoors / ModifyWindows), an object slides along the wall
it stands against, or moves straight away from the obstruction when it stands
against none (MoveElements). Each is moved by the smallest amount that clears
it; one that does not clear within its wall, or would run into another door,
window or checked object, is listed and not moved.

The margin comes, in this order, from a property picked in the window, from a
table per library part, or from the default of its kind. The settings and the
table are saved in Documents/Tapir/WallClearance.json; the table can also be
saved to and loaded from a file of its own, for the office's standards.
"""

import fnmatch
import json
import math
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, simpledialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Wall Clearance"
EPSILON = 1e-6
TOLERANCE = 0.0005
# how far behind an object's back edge (and in front of it) a wall face still counts as the wall it stands against
BACK_TOLERANCE = 0.05
# a face this close to parallel to the object's back edge can be the wall it stands against
PARALLEL_DEGREES = 20.0
# the step of the search for the smallest move; the found move is refined to well below a millimetre
SEARCH_STEP = 0.01
# how far an object that stands against no wall may move away from an obstruction
MAX_FREE_MOVE = 1.0
ARC_SEGMENTS = 32

TOO_CLOSE_COLOR = [230, 140, 0, 255]
PROBLEM_COLOR = [200, 40, 40, 255]
MOVED_COLOR = [0, 130, 200, 255]
DIMMED_COLOR = [200, 200, 200, 160]

KINDS = ["Door", "Window", "Object"]
KIND_LABELS = {"Door": "Doors", "Window": "Windows", "Object": "Objects"}
DEFAULT_MARGINS = {"Door": "0.10", "Window": "0.10", "Object": "0.20"}
NO_PROPERTY = "(none)"


def settings_folder() -> Path:
    """ Documents/Tapir, where Tapir keeps its own config.json as well. """
    documents = Path.home() / "Documents"
    return (documents if documents.is_dir() else Path.home()) / "Tapir"


SETTINGS_FILE = settings_folder() / "WallClearance.json"


def parse_length(text) -> float | None:
    try:
        value = float(str(text).strip().replace(",", "."))
    except ValueError:
        return None
    return value if math.isfinite(value) else None


def fmt(value: float | None) -> str:
    return "" if value is None else f"{value:.3f}"


# ---- geometry ----

def _xy(point: dict) -> tuple[float, float]:
    return float(point["x"]), float(point["y"])


def read_polygon(coordinates: list[dict]) -> list[tuple[float, float]]:
    points = [_xy(p) for p in coordinates or []]
    if len(points) > 1 and abs(points[0][0] - points[-1][0]) < EPSILON and abs(points[0][1] - points[-1][1]) < EPSILON:
        points.pop()
    return points


def edges(points):
    return zip(points, points[1:] + points[:1])


def signed_area(points) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for a, b in edges(points)) / 2.0


def inside(p, points) -> bool:
    result = False
    for a, b in edges(points):
        if (a[1] > p[1]) != (b[1] > p[1]):
            x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
            if p[0] < x:
                result = not result
    return result


def bbox(points) -> tuple[float, float, float, float]:
    return (min(p[0] for p in points), min(p[1] for p in points), max(p[0] for p in points), max(p[1] for p in points))


def centroid(points) -> tuple[float, float]:
    return sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points)


def convex_hull(points) -> list[tuple[float, float]]:
    """ The convex hull, counterclockwise (Andrew's monotone chain). """
    pts = sorted(set((round(x, 6), round(y, 6)) for x, y in points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def shifted(points, d, t):
    return [(x + d[0] * t, y + d[1] * t) for x, y in points]


def closest_on_segment(p, a, b) -> tuple[float, float]:
    dx, dy = b[0] - a[0], b[1] - a[1]
    length2 = dx * dx + dy * dy
    r = 0.0 if length2 < EPSILON * EPSILON else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length2))
    return a[0] + dx * r, a[1] + dy * r


def point_segment_distance(p, a, b) -> float:
    c = closest_on_segment(p, a, b)
    return math.hypot(p[0] - c[0], p[1] - c[1])


def closest_on_boundary(p, points) -> tuple[float, float]:
    return min((closest_on_segment(p, a, b) for a, b in edges(points)),
               key=lambda c: (c[0] - p[0]) ** 2 + (c[1] - p[1]) ** 2)


def boundary_distance(p, points) -> float:
    return min(point_segment_distance(p, a, b) for a, b in edges(points))


def segments_cross(a, b, c, d) -> bool:
    def orient(p, q, r):
        v = (q[0] - p[0]) * (r[1] - p[1]) - (q[1] - p[1]) * (r[0] - p[0])
        return 0 if abs(v) < EPSILON * EPSILON else (1 if v > 0 else -1)
    return orient(a, b, c) * orient(a, b, d) < 0 and orient(c, d, a) * orient(c, d, b) < 0


def polygon_gap(first, second, limit: float = math.inf) -> float:
    """ The smallest distance between two polygons; when they overlap, minus how deep one reaches into the other.
    Returns limit (or more) without computing when their bounding boxes are at least limit apart. """
    b1, b2 = bbox(first), bbox(second)
    box_gap = max(b2[0] - b1[2], b1[0] - b2[2], b2[1] - b1[3], b1[1] - b2[3])
    if box_gap >= limit:
        return box_gap
    depth = 0.0
    overlap = False
    for points, other in ((first, second), (second, first)):
        for p in points:
            if inside(p, other):
                overlap = True
                depth = max(depth, boundary_distance(p, other))
    if not overlap:
        overlap = any(segments_cross(a, b, c, d) for a, b in edges(first) for c, d in edges(second))
    if overlap:
        return -depth
    return min(min(point_segment_distance(p, a, b) for p in first for a, b in edges(second)),
               min(point_segment_distance(p, a, b) for p in second for a, b in edges(first)))


def arc_points(primitive: dict) -> list[tuple[float, float]]:
    cx, cy = _xy(primitive["orig"])
    r = float(primitive.get("r", 0.0))
    ratio = float(primitive.get("ratio") or 1.0)
    rotation = float(primitive.get("angle") or 0.0)
    if primitive.get("whole"):
        start, sweep = 0.0, 2 * math.pi
    else:
        start, end = float(primitive.get("begAng", 0.0)), float(primitive.get("endAng", 0.0))
        if primitive.get("reflected"):
            # a clockwise arc from begAng to endAng covers the counterclockwise one from endAng to begAng
            start, end = end, start
        sweep = (end - start) % (2 * math.pi) or 2 * math.pi
    steps = max(2, math.ceil(ARC_SEGMENTS * sweep / (2 * math.pi)))
    cos_r, sin_r = math.cos(rotation), math.sin(rotation)
    result = []
    for i in range(steps + 1):
        angle = start + sweep * i / steps
        x, y = r * math.cos(angle), r * ratio * math.sin(angle)
        result.append((cx + x * cos_r - y * sin_r, cy + x * sin_r + y * cos_r))
    return result


def outline_from_primitives(primitives: list[dict]) -> list[tuple[float, float]]:
    """ The convex hull of the lines, arcs, fills and pictures Archicad draws the element from (texts and hotspots
    left out), in plan coordinates; empty when it draws none. """
    points = []
    in_text = False
    for primitive in primitives or []:
        kind = primitive.get("type")
        if kind == "Ctrl_TextBeg":
            in_text = True
        elif kind == "Ctrl_TextEnd":
            in_text = False
        elif in_text:
            continue
        elif kind == "Line":
            points += [_xy(primitive["c1"]), _xy(primitive["c2"])]
        elif kind == "Arc":
            points += arc_points(primitive)
        elif kind == "Tri":
            points += [_xy(p) for p in primitive.get("coordinates") or []]
        elif kind == "Poly":
            polygons = primitive.get("polygons") or []
            if polygons:
                points += [_xy(p) for p in polygons[0].get("coordinates") or []]
        elif kind == "Pict" and primitive.get("destBox"):
            box = primitive["destBox"]
            points += [(box["xMin"], box["yMin"]), (box["xMax"], box["yMax"]),
                       (box["xMin"], box["yMax"]), (box["xMax"], box["yMin"])]
    hull = convex_hull(points)
    return hull if len(hull) > 2 else []


class Obstruction:
    """ An element that shows cut in the floor plan (a wall, a column or a curtain wall): its body is its floor plan
    cut fill. """

    length = 0.0
    curved = False
    approximate = False

    def __init__(self, element_id: dict, detail: dict, kind: str = "element"):
        self.elementId = element_id
        self.guid = element_id["guid"]
        self.floorIndex = int(detail.get("floorIndex", 0))
        self.label = detail.get("id") or kind
        self.polygons = [p for p in (read_polygon(item.get("coordinates")) for item in detail.get("floorPlanPolygons") or [])
                         if len(p) > 2]

    def faces(self):
        """ (a, b, outward unit normal) for each edge of the body. """
        for polygon in self.polygons:
            turn = 1.0 if signed_area(polygon) > 0 else -1.0
            for a, b in edges(polygon):
                length = math.hypot(b[0] - a[0], b[1] - a[1])
                if length > EPSILON:
                    yield a, b, (turn * (b[1] - a[1]) / length, -turn * (b[0] - a[0]) / length)


class Wall(Obstruction):
    """ A wall with its reference line (from begCoordinate to endCoordinate) as the t axis and its left normal as
    the s axis, and its body: the floor plan cut fill polygons, or a rectangle around the reference line when
    Archicad gives none. """

    def __init__(self, element_id: dict, detail: dict):
        super().__init__(element_id, detail, "wall")
        d = detail.get("details") or {}
        self.beg = _xy(d["begCoordinate"])
        end = _xy(d["endCoordinate"])
        self.length = math.hypot(end[0] - self.beg[0], end[1] - self.beg[1])
        self.u = ((end[0] - self.beg[0]) / self.length, (end[1] - self.beg[1]) / self.length) \
            if self.length > EPSILON else (1.0, 0.0)
        self.n = (-self.u[1], self.u[0])
        self.curved = d.get("geometryType") == "Straight" and abs(float(d.get("arcAngle") or 0.0)) > EPSILON
        self.approximate = not self.polygons
        if self.approximate and self.length > EPSILON:
            thickness = max(float(d.get("begThickness") or 0.0), float(d.get("endThickness") or 0.0)) or 0.3
            centered = d.get("referenceLineLocation") in ("Center", "CoreCenter")
            low, high = (-thickness / 2, thickness / 2) if centered else (-thickness, thickness)
            self.polygons = [[self.world(0.0, low), self.world(self.length, low),
                              self.world(self.length, high), self.world(0.0, high)]]
        local = [self.local(p) for polygon in self.polygons for p in polygon]
        self.t_range = (min(p[0] for p in local), max(p[0] for p in local)) if local else (0.0, self.length)

    def local(self, p) -> tuple[float, float]:
        dx, dy = p[0] - self.beg[0], p[1] - self.beg[1]
        return dx * self.u[0] + dy * self.u[1], dx * self.n[0] + dy * self.n[1]

    def world(self, t: float, s: float) -> tuple[float, float]:
        return (self.beg[0] + self.u[0] * t + self.n[0] * s, self.beg[1] + self.u[1] * t + self.n[1] * s)


def nearest_obstruction(outline, obstructions: list[Obstruction]) -> tuple[float, Obstruction] | None:
    found = None
    for obstruction in obstructions:
        for polygon in obstruction.polygons:
            gap = polygon_gap(outline, polygon, math.inf if found is None else found[0])
            if found is None or gap < found[0]:
                found = (gap, obstruction)
    return found


def side_of(outline, obstruction: Obstruction, front) -> str:
    """ Which side of the element the obstruction is on, seen from the element facing its front. """
    center = centroid(outline)
    point = min((closest_on_boundary(center, polygon) for polygon in obstruction.polygons),
                key=lambda c: (c[0] - center[0]) ** 2 + (c[1] - center[1]) ** 2)
    v = (point[0] - center[0], point[1] - center[1])
    ahead = v[0] * front[0] + v[1] * front[1]
    leftward = -v[0] * front[1] + v[1] * front[0]
    if abs(ahead) >= abs(leftward):
        return "front" if ahead > 0 else "back"
    return "left" if leftward > 0 else "right"


def smallest_move(t_low: float, t_high: float, clears) -> float | None:
    """ The move t (t_low <= t <= t_high) of the smallest size for which clears(t) holds: searched in SEARCH_STEP
    steps on both sides of 0, then refined between the last step that does not clear and the first one that does.
    None when no step clears. """
    if t_low > t_high:
        return None
    limit = max(abs(t_low), abs(t_high))
    tried = set()
    for k in range(1, math.ceil(limit / SEARCH_STEP) + 2):
        for sign in (1.0, -1.0):
            t = max(t_low, min(t_high, sign * k * SEARCH_STEP))
            if t in tried or (t != 0.0 and (t > 0) != (sign > 0)):
                continue
            tried.add(t)
            if not clears(t):
                continue
            fails = max(t_low, min(t_high, sign * (k - 1) * SEARCH_STEP))
            for _ in range(14):
                middle = (fails + t) / 2
                if clears(middle):
                    t = middle
                else:
                    fails = middle
            # the refined move ends right at the edge of the tolerance, so a check after the move could find the
            # gap just short of the margin again: go a little further when that still clears
            further = max(t_low, min(t_high, t + math.copysign(2 * TOLERANCE, t)))
            return further if clears(further) else t
    return None


class WallClearance:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds the doors, windows and objects whose floor plan outline is too close to a wall or column and "
            "moves them just enough. Set the margins, press Check to see the list, then Apply.",
            min_width=980, min_height=760)
        self.properties: dict[str, dict] = {}
        self.table: list[dict] = []
        self.rows: list[dict] = []
        self.checked_snapshot = None
        self.applied = False
        self.margin_vars: dict[str, tk.StringVar] = {}
        self._build_body()
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Select in Archicad", self.select_rows)
        self.window.add_button("Apply", self.apply)
        self.window.add_button("Check", self.check, primary=True)

    # ---- window ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        scope = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        scope.grid(row=0, column=0, sticky="ew")
        scope.columnconfigure(6, weight=1)
        self.scope = tk.StringVar(value="selection")
        ttk.Radiobutton(scope, text="Selection", value="selection", variable=self.scope).grid(row=0, column=0, sticky="w")
        ttk.Radiobutton(scope, text="Current story", value="story", variable=self.scope).grid(
            row=0, column=1, sticky="w", padx=(10, 0))
        self.kind_vars = {kind: tk.BooleanVar(value=True) for kind in KINDS}
        for column, kind in enumerate(KINDS, start=2):
            ttk.Checkbutton(scope, text=KIND_LABELS[kind], variable=self.kind_vars[kind]).grid(
                row=0, column=column, sticky="w", padx=(24 if column == 2 else 10, 0))
        ttk.Label(scope, text="Library part name contains:").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.part_filter = tk.StringVar()
        ttk.Entry(scope, textvariable=self.part_filter).grid(row=1, column=2, columnspan=5, sticky="ew", pady=(6, 0))

        margins = ttk.LabelFrame(body, text="Margin around the element's floor plan outline (m)", padding=(10, 6))
        margins.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        margins.columnconfigure(7, weight=1)
        for index, kind in enumerate(KINDS):
            ttk.Label(margins, text=KIND_LABELS[kind] + ":").grid(row=0, column=index * 2, sticky="w",
                                                                  padx=(0 if index == 0 else 16, 0))
            var = tk.StringVar(value=DEFAULT_MARGINS[kind])
            ttk.Entry(margins, textvariable=var, width=8).grid(row=0, column=index * 2 + 1, sticky="w", padx=(6, 0))
            self.margin_vars[kind] = var
        ttk.Label(margins, text="Take the margin from a property:").grid(row=1, column=0, columnspan=2,
                                                                          sticky="w", pady=(6, 0))
        self.property = tk.StringVar(value=NO_PROPERTY)
        self.property_box = ttk.Combobox(margins, textvariable=self.property, values=[NO_PROPERTY], state="readonly",
                                         width=50)
        self.property_box.grid(row=1, column=2, columnspan=6, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(margins, text="An element whose property has a number uses it instead of the table and the "
                                "margin of its kind. Length values are in metres.", style="Muted.TLabel").grid(
            row=2, column=0, columnspan=8, sticky="w")

        table = ttk.LabelFrame(body, text="Per library part (the first matching row wins; * matches any text)",
                               padding=(10, 6))
        table.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        table.columnconfigure(0, weight=1)
        self.table_tree = make_tree(table, [("pattern", "Library part name", 460, "w"),
                                            ("margin", "Margin", 110, "e")], height=4)
        self.table_tree.container.grid(row=0, column=0, sticky="ew")
        self.table_tree.bind("<Double-1>", lambda _e: self.window.guarded(self.edit_row)())
        buttons = ttk.Frame(table)
        buttons.grid(row=0, column=1, sticky="n", padx=(8, 0))
        for text, command in (("Add...", self.add_row), ("Edit...", self.edit_row), ("Remove", self.remove_row),
                              ("Load...", self.load_table), ("Save...", self.save_table)):
            ttk.Button(buttons, text=text, command=self.window.guarded(command)).pack(fill="x", pady=(0, 3))

        results = ttk.Frame(body, padding=(0, 8, 0, 0))
        results.grid(row=3, column=0, sticky="nsew")
        results.columnconfigure(0, weight=1)
        results.rowconfigure(1, weight=1)
        self.summary = ttk.Label(results, text="Press Check to find the elements that are too close to a wall.")
        self.summary.grid(row=0, column=0, sticky="w", pady=(0, 4))
        self.tree = make_tree(results, [("element", "Element", 140, "w"), ("part", "Library part", 160, "w"),
                                        ("story", "Story", 80, "w"), ("obstruction", "Obstruction", 140, "w"),
                                        ("side", "Side", 50, "w"), ("now", "Now", 60, "e"),
                                        ("needed", "Needed", 60, "e"), ("move", "Move", 60, "e"),
                                        ("result", "Result", 230, "w")], height=10)
        self.tree.container.grid(row=1, column=0, sticky="nsew")
        self.tree.tag_configure("fix", foreground="#a15c00")
        self.tree.tag_configure("problem", foreground="#c62828")
        self.tree.tag_configure("skipped", foreground="#777777")
        self.tree.tag_configure("moved", foreground="#0060a0")
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_rows)())

    # ---- the per library part table ----

    def _list_table(self) -> None:
        self.table_tree.delete(*self.table_tree.get_children())
        for index, row in enumerate(self.table):
            self.table_tree.insert("", "end", iid=str(index), values=(row["pattern"], fmt(row["margin"])))

    def _ask_row(self, row: dict | None) -> dict | None:
        root = self.window.root
        pattern = simpledialog.askstring(SCRIPT_NAME, "Library part name (* and ? match any text, e.g. *WC*):",
                                         initialvalue=row["pattern"] if row else "", parent=root)
        if not pattern or not pattern.strip():
            return None
        margin_text = simpledialog.askstring(SCRIPT_NAME, "Margin around its floor plan outline (m):",
                                             initialvalue=fmt(row["margin"]) if row else "", parent=root)
        if margin_text is None:
            return None
        margin = parse_length(margin_text)
        if margin is None or margin < 0:
            self.window.set_status(f"'{margin_text}' is not a distance in metres.", "warning")
            return None
        return {"pattern": pattern.strip(), "margin": margin}

    def add_row(self) -> None:
        row = self._ask_row(None)
        if row:
            self.table.append(row)
            self._list_table()

    def edit_row(self) -> None:
        selected = self.table_tree.selection()
        if not selected:
            self.window.set_status("Pick a row of the table first.", "warning")
            return
        index = int(selected[0])
        row = self._ask_row(self.table[index])
        if row:
            self.table[index] = row
            self._list_table()

    def remove_row(self) -> None:
        for index in sorted((int(i) for i in self.table_tree.selection()), reverse=True):
            del self.table[index]
        self._list_table()

    @staticmethod
    def _read_table(data) -> list[dict]:
        rows = []
        for item in data if isinstance(data, list) else []:
            if not isinstance(item, dict) or not str(item.get("pattern", "")).strip():
                continue
            # "first" is the key of tables saved by the first version of this script
            margin = parse_length(item.get("margin", item.get("first")))
            if margin is not None and margin >= 0:
                rows.append({"pattern": str(item["pattern"]).strip(), "margin": margin})
        return rows

    def load_table(self) -> None:
        path = filedialog.askopenfilename(parent=self.window.root, title="Load the margins",
                                          filetypes=[("Wall Clearance table", "*.json"), ("All files", "*.*")])
        if not path:
            return
        try:
            with open(path, encoding="utf-8") as file:
                data = json.load(file)
        except (OSError, ValueError) as e:
            self.window.set_status(f"Cannot read {path}: {e}", "error")
            return
        if not isinstance(data, dict):
            self.window.set_status(f"{path} is not a Wall Clearance table.", "error")
            return
        self.table = self._read_table(data.get("table"))
        self._apply_margins(data.get("margins"))
        self._list_table()
        self.window.set_status(f"Loaded {len(self.table)} row(s) and the margins from {path}.", "ok")

    def save_table(self) -> None:
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the margins",
                                            defaultextension=".json", initialfile="Wall Clearance.json",
                                            filetypes=[("Wall Clearance table", "*.json")])
        if not path:
            return
        data = {"margins": self._margin_texts(), "table": self.table}
        try:
            with open(path, "w", encoding="utf-8") as file:
                json.dump(data, file, indent=2, ensure_ascii=False)
        except OSError as e:
            self.window.set_status(f"Cannot save {path}: {e}", "error")
            return
        self.window.set_status(f"Saved the margins and {len(self.table)} row(s) to {path}.", "ok")

    # ---- settings ----

    def _margin_texts(self) -> dict:
        return {kind: var.get() for kind, var in self.margin_vars.items()}

    def _apply_margins(self, data) -> None:
        if not isinstance(data, dict):
            return
        for kind, var in self.margin_vars.items():
            if isinstance(data.get(kind), (str, int, float)):
                var.set(str(data[kind]))

    def _settings_data(self) -> dict:
        prop = self.properties.get(self.property.get())
        return {
            "scope": self.scope.get(),
            "kinds": {kind: var.get() for kind, var in self.kind_vars.items()},
            "partFilter": self.part_filter.get(),
            "margins": self._margin_texts(),
            "property": {"label": self.property.get(), "guid": prop["propertyId"]["guid"]} if prop else None,
            "table": self.table,
        }

    def load_settings(self) -> None:
        try:
            with open(SETTINGS_FILE, encoding="utf-8") as file:
                data = json.load(file)
        except (OSError, ValueError):
            return
        if not isinstance(data, dict):
            return
        if data.get("scope") in ("selection", "story"):
            self.scope.set(data["scope"])
        for kind, var in self.kind_vars.items():
            if isinstance(data.get("kinds"), dict) and kind in data["kinds"]:
                var.set(bool(data["kinds"][kind]))
        self.part_filter.set(str(data.get("partFilter") or ""))
        self._apply_margins(data.get("margins"))
        self.table = self._read_table(data.get("table"))
        self._list_table()
        prop = data.get("property")
        if isinstance(prop, dict):
            for label, p in self.properties.items():
                if p["propertyId"].get("guid") == prop.get("guid"):
                    self.property.set(label)
                    break

    def save_settings(self) -> None:
        try:
            SETTINGS_FILE.parent.mkdir(parents=True, exist_ok=True)
            with open(SETTINGS_FILE, "w", encoding="utf-8") as file:
                json.dump(self._settings_data(), file, indent=2, ensure_ascii=False)
        except OSError as e:
            print(f"Cannot save the settings to {SETTINGS_FILE}: {e}")

    def _read_settings(self) -> dict | None:
        """ The parsed settings, or None after showing what is wrong. """
        kinds = [kind for kind, var in self.kind_vars.items() if var.get()]
        if not kinds:
            self.window.set_status("Tick Doors, Windows or Objects.", "warning")
            return None
        margins = {}
        for kind, var in self.margin_vars.items():
            value = parse_length(var.get())
            if value is None or value < 0:
                self.window.set_status(f"The margin of the {KIND_LABELS[kind].lower()} must be a non-negative "
                                       "number of metres.", "warning")
                return None
            margins[kind] = value
        prop = self.properties.get(self.property.get())
        return {"scope": self.scope.get(), "kinds": kinds, "partFilter": self.part_filter.get().lower().split(),
                "margins": margins, "property": prop["propertyId"] if prop else None,
                "table": [dict(row) for row in self.table]}

    # ---- start ----

    def load(self) -> None:
        with self.window.busy("Reading the properties..."):
            try:
                properties = run_tapir("GetAllProperties").get("properties", [])
            except ArchicadError as e:
                print(f"Cannot read the properties: {e}")
                properties = []
        self.properties = {}
        for p in sorted((p for p in properties if "propertyId" in p and p.get("propertyType") == "Custom"),
                        key=lambda p: (p.get("propertyGroupName", "").lower(), p.get("propertyName", "").lower())):
            label = f"{p.get('propertyGroupName', '')} / {p.get('propertyName', '')}"
            if label in self.properties:
                label = f"{label} ({p['propertyId']['guid'][:8]})"
            self.properties[label] = p
        self.property_box.configure(values=[NO_PROPERTY] + list(self.properties))
        self.load_settings()
        self.window.set_status("Set the margins and press Check.")

    # ---- reading ----

    def _property_values(self, elements: list[dict], property_id: dict) -> list[float | None]:
        values: list[float | None] = []
        use_si = True
        for chunk in batched(elements):
            parameters = {"elements": chunk, "properties": [{"propertyId": property_id}]}
            if use_si:
                try:
                    result = run_tapir("GetPropertyValuesOfElements", {**parameters, "useSIUnits": True})
                except ArchicadError:
                    # Add-Ons before useSIUnits reject the parameter; they give the displayed values
                    use_si = False
                    result = run_tapir("GetPropertyValuesOfElements", parameters)
            else:
                result = run_tapir("GetPropertyValuesOfElements", parameters)
            for entry in result.get("propertyValuesForElements", []):
                item = (entry.get("propertyValues") or [{}])[0] if "error" not in entry else {}
                value = parse_length((item.get("propertyValue") or {}).get("value", ""))
                values.append(value if value is not None and value >= 0 else None)
        if len(values) != len(elements):
            raise ArchicadError("Archicad returned a different number of property values than elements asked for.")
        return values

    def _read_obstructions(self, floors: set[int]) -> list[Obstruction]:
        """ The walls, columns and curtain walls of the given stories. Columns and curtain walls count only by their
        floor plan cut fill; one without it is left out. """
        obstructions: list[Obstruction] = []
        for kind in ("Wall", "Column", "CurtainWall"):
            try:
                ids = run_tapir("GetElementsByType", {"elementType": kind}).get("elements", [])
            except ArchicadError as e:
                if kind == "Wall":
                    raise
                print(f"Cannot read the {kind} elements: {e}")
                continue
            floor_details = get_details_of_elements(ids, ["floorIndex"])
            ids = [i for i, d in zip(ids, floor_details) if int(d.get("floorIndex", 0)) in floors]
            if kind == "Wall":
                details = get_details_of_elements(ids, ["type", "id", "floorIndex", "details", "floorPlanPolygons"])
                for wall_id, detail in zip(ids, details):
                    d = detail.get("details") or {}
                    if detail.get("type") == "Wall" and "begCoordinate" in d and "endCoordinate" in d:
                        obstructions.append(Wall(wall_id["elementId"], detail))
            else:
                label = "column" if kind == "Column" else "curtain wall"
                details = get_details_of_elements(ids, ["type", "id", "floorIndex", "floorPlanPolygons"])
                for element_id, detail in zip(ids, details):
                    obstruction = Obstruction(element_id["elementId"], detail, label)
                    if obstruction.polygons:
                        obstructions.append(obstruction)
        return obstructions

    @staticmethod
    def _read_outlines(command: str, key: str, elements: list[dict], extra: dict) -> list[tuple[list, str]]:
        """ (outline, error) of each element from the 2D primitives the command returns. """
        result = []
        for chunk in batched([{"elementId": e["elementId"]} for e in elements]):
            items = run_tapir(command, {"elements": chunk, **extra}).get(key, [])
            for item in items:
                if not isinstance(item, dict) or "error" in item:
                    error = (item.get("error") or {}).get("message", "") if isinstance(item, dict) else ""
                    result.append(([], error))
                else:
                    result.append((outline_from_primitives(item.get("primitives")), ""))
        result += [([], "no result")] * (len(elements) - len(result))
        return result

    def _margin_for(self, kind: str, part: str, value: float | None, settings: dict) -> float:
        if value is not None:
            return value
        name = part.lower()
        for row in settings["table"]:
            if fnmatch.fnmatchcase(name, row["pattern"].lower()):
                return row["margin"]
        return settings["margins"][kind]

    # ---- checking ----

    def check(self) -> bool:
        settings = self._read_settings()
        if settings is None:
            return False
        self.save_settings()
        with self.window.busy("Reading the elements..."):
            element_ids = get_elements(settings["scope"], settings["kinds"])
            details = get_details_of_elements(element_ids, ["type", "id", "floorIndex", "details"])
            elements = []
            for element_id, detail in zip(element_ids, details):
                d = detail.get("details") or {}
                part = (d.get("libPart") or {}).get("name", "")
                if detail.get("type") not in settings["kinds"]:
                    continue
                if settings["partFilter"] and not all(w in part.lower() for w in settings["partFilter"]):
                    continue
                if detail.get("type") == "Object" and d.get("ownerElementId"):
                    continue
                elements.append({"elementId": element_id["elementId"], "kind": detail["type"], "part": part,
                                 "label": detail.get("id") or part or detail["type"],
                                 "floorIndex": int(detail.get("floorIndex", 0)), "details": d})
            if not elements:
                self.rows = []
                self.checked_snapshot = None
                self._show_rows()
                run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
                self.window.set_status("There is nothing to check: select doors, windows or objects in Archicad, "
                                       "or pick Current story." if settings["scope"] == "selection"
                                       else "There are no matching doors, windows or objects on the current story.",
                                       "warning")
                return False

            values = [None] * len(elements)
            if settings["property"]:
                self.window.set_status("Reading the property values...")
                values = self._property_values([{"elementId": e["elementId"]} for e in elements], settings["property"])
            for element, value in zip(elements, values):
                element["margin"] = self._margin_for(element["kind"], element["part"], value, settings)

            self.window.set_status("Reading the floor plan outlines...")
            openings = [e for e in elements if e["kind"] in ("Door", "Window")]
            objects = [e for e in elements if e["kind"] == "Object"]
            try:
                # GetShapePrimitivesOfElements gives no primitives for doors and windows (Archicad draws them as part
                # of their wall), so they are measured by their opening in the wall (see _opening_outline). Objects
                # are drawn on every story.
                for element, (outline, error) in zip(openings, self._read_outlines(
                        "GetShapePrimitivesOfElements", "shapePrimitivesOfElements", openings, {"includeMarkers": False})):
                    element["outline"], element["outlineError"] = outline, error
                for element, (outline, error) in zip(objects, self._read_outlines(
                        "GetDrawingPrimitivesOfElements", "drawingPrimitivesOfElements", objects, {})):
                    element["outline"], element["outlineError"] = outline, error
            except ArchicadError as e:
                self.window.set_status(f"Cannot read the floor plan outlines ({e}). The floor plan must be the current "
                                       "window, and the Tapir Add-On 1.8.1 or later is needed.", "error")
                return False

            self.window.set_status("Reading the walls, columns and curtain walls...")
            obstructions = self._read_obstructions({e["floorIndex"] for e in elements})
            stories = get_story_names()
            rows = []
            if openings:
                self.window.set_status("Measuring the doors and windows...")
                rows += self._check_openings(openings, obstructions)
            if objects:
                self.window.set_status("Measuring the objects...")
                rows += self._check_objects(objects, obstructions)
            for row in rows:
                row["story"] = stories.get(row["floorIndex"], str(row["floorIndex"]))

        order = {"problem": 0, "fix": 1, "skipped": 2}
        self.rows = sorted(rows, key=lambda r: (order[r["status"]], r["floorIndex"], r["label"].lower()))
        self.checked_snapshot = json.dumps(self._settings_data(), sort_keys=True)
        self.applied = False
        self._show_rows()
        self._highlight()
        fix = sum(1 for r in self.rows if r["status"] == "fix")
        problems = sum(1 for r in self.rows if r["status"] == "problem")
        skipped = sum(1 for r in self.rows if r["status"] == "skipped")
        message = (f"Checked {len(elements)} element(s): {fix} to move, {problems} that cannot be moved, "
                   f"{skipped} not checked, {len(elements) - fix - problems - skipped} clear of the walls.")
        if any(o.approximate for o in obstructions):
            message += " Some walls have no floor plan outline; they are measured from their reference line."
        self.window.set_status(message, "warning" if fix or problems else "ok")
        return True

    @staticmethod
    def _clears(outline, margin: float, obstructions: list[Obstruction], others: list) -> bool:
        """ Whether the outline keeps the margin from every obstruction and does not run into another element. """
        for obstruction in obstructions:
            for polygon in obstruction.polygons:
                if polygon_gap(outline, polygon, margin) < margin - TOLERANCE:
                    return False
        return all(polygon_gap(outline, other, TOLERANCE) > -TOLERANCE for other in others)

    @staticmethod
    def _near(outline, obstructions: list[Obstruction], reach: float) -> list[Obstruction]:
        """ The obstructions within reach of the outline's bounding box, so the search measures only those. """
        x0, y0, x1, y1 = bbox(outline)
        result = []
        for obstruction in obstructions:
            for polygon in obstruction.polygons:
                b = bbox(polygon)
                if b[0] < x1 + reach and b[2] > x0 - reach and b[1] < y1 + reach and b[3] > y0 - reach:
                    result.append(obstruction)
                    break
        return result

    def _check_openings(self, openings: list[dict], obstructions: list[Obstruction]) -> list[dict]:
        wall_by_guid = {o.guid: o for o in obstructions if isinstance(o, Wall)}
        # every door and window in the owner walls, so a moved one does not run into another
        positions: dict[str, dict] = {}
        owners = {(e["details"].get("ownerElementId") or {}).get("guid") for e in openings}
        for kind in ("Door", "Window"):
            try:
                ids = run_tapir("GetElementsByType", {"elementType": kind}).get("elements", [])
            except ArchicadError as e:
                print(f"Cannot read the {kind.lower()}s: {e}")
                continue
            for element_id, detail in zip(ids, get_details_of_elements(ids, ["id", "details"])):
                d = detail.get("details") or {}
                owner = (d.get("ownerElementId") or {}).get("guid")
                if owner in owners and "centerOffset" in d:
                    half = float(d.get("width", 0.0)) / 2.0
                    positions[element_id["elementId"]["guid"]] = {
                        "owner": owner, "label": detail.get("id") or kind.lower(),
                        "low": float(d["centerOffset"]) - half, "high": float(d["centerOffset"]) + half}

        rows = []
        for element in openings:
            d = element["details"]
            row = self._row(element)
            owner = wall_by_guid.get((d.get("ownerElementId") or {}).get("guid"))
            if owner is None or "centerOffset" not in d:
                rows.append({**row, "status": "skipped", "result": "Not in a wall; not checked."})
                continue
            if owner.curved:
                rows.append({**row, "status": "skipped", "result": "In a curved wall; not checked."})
                continue
            outline = element["outline"] or self._opening_outline(owner, d)
            if not outline:
                reason = element["outlineError"] or "it has no width"
                rows.append({**row, "status": "skipped",
                             "result": f"Its space in the wall cannot be worked out ({reason}); not checked."})
                continue
            margin = element["margin"]
            active = [o for o in obstructions if o.floorIndex == owner.floorIndex and o.guid != owner.guid]
            nearest = nearest_obstruction(outline, active)
            if nearest is None or nearest[0] >= margin - TOLERANCE:
                continue
            offset = (centroid(outline)[0] - owner.beg[0]) * owner.n[0] + (centroid(outline)[1] - owner.beg[1]) * owner.n[1]
            front = owner.n if offset >= 0 else (-owner.n[0], -owner.n[1])
            row.update(obstruction=nearest[1].label, side=side_of(outline, nearest[1], front), now=nearest[0],
                       needed=margin)

            guid = element["elementId"]["guid"]
            center = float(d["centerOffset"])
            half = float(d.get("width", 0.0)) / 2.0
            low, high = center - half, center + half
            t_low = max(owner.t_range[0] - low, -center)
            t_high = owner.t_range[1] - high
            neighbours = [p for g, p in positions.items() if g != guid and p["owner"] == owner.guid]
            # it slides up to the next door or window in its wall, never past it
            for p in neighbours:
                if p["low"] >= high - TOLERANCE:
                    t_high = min(t_high, p["low"] - high)
                elif p["high"] <= low + TOLERANCE:
                    t_low = max(t_low, p["high"] - low)
            near = self._near(outline, active, max(abs(t_low), abs(t_high)) + margin)

            def clears(t):
                if any(p["high"] > low + t + TOLERANCE and p["low"] < high + t - TOLERANCE for p in neighbours):
                    return False
                return self._clears(shifted(outline, owner.u, t), margin, near, [])

            shift = smallest_move(t_low, t_high, clears)
            if shift is None:
                rows.append({**row, "status": "problem",
                             "result": "Sliding it along its wall does not clear it within the wall's length "
                                       "without running into another door or window."})
                continue
            row["move"] = abs(shift)
            if guid in positions:
                positions[guid].update(low=low + shift, high=high + shift)
            rows.append({**row, "status": "fix", "result": "", "action": {"centerOffset": center + shift}})
        return rows

    @staticmethod
    def _opening_outline(wall: Wall, d: dict) -> list[tuple[float, float]]:
        """ The opening of a door or window in its wall: from centerOffset - width / 2 to centerOffset + width / 2
        along the wall, across the wall's body. GetShapePrimitivesOfElements gives no primitives for doors and
        windows (Archicad draws them as part of their wall), and their 2D bounding box is this opening too. """
        width = float(d.get("width") or 0.0)
        local = [wall.local(p) for polygon in wall.polygons for p in polygon]
        if width <= EPSILON or not local:
            return []
        s_low, s_high = min(p[1] for p in local), max(p[1] for p in local)
        low, high = float(d["centerOffset"]) - width / 2.0, float(d["centerOffset"]) + width / 2.0
        return [wall.world(low, s_low), wall.world(high, s_low), wall.world(high, s_high), wall.world(low, s_high)]

    @staticmethod
    def _back_face(element: dict, outline, obstructions: list[Obstruction]):
        """ The face (a, b, outward normal) and obstruction the object stands against: the face nearest the back edge
        of its outline (the side its insertion point and rotation face), parallel to it and facing the object,
        within BACK_TOLERANCE. None when it stands against none. """
        d = element["details"]
        angle = float(d.get("angle") or 0.0)
        origin = _xy(d.get("origin") or {"x": centroid(outline)[0], "y": centroid(outline)[1]})
        along = (math.cos(angle), math.sin(angle))
        front = (-math.sin(angle), math.cos(angle))
        back = min((p[0] - origin[0]) * front[0] + (p[1] - origin[1]) * front[1] for p in outline)
        extent = [(p[0] - origin[0]) * along[0] + (p[1] - origin[1]) * along[1] for p in outline]
        parallel = math.cos(math.radians(PARALLEL_DEGREES))
        found = None
        for obstruction in obstructions:
            for a, b, m in obstruction.faces():
                if m[0] * front[0] + m[1] * front[1] < parallel:
                    continue
                # where the face is, measured from the back edge towards the front (negative: behind it, positive:
                # the object reaches into the wall)
                position = (a[0] - origin[0]) * front[0] + (a[1] - origin[1]) * front[1] - back
                if abs(position) > BACK_TOLERANCE:
                    continue
                fa = (a[0] - origin[0]) * along[0] + (a[1] - origin[1]) * along[1]
                fb = (b[0] - origin[0]) * along[0] + (b[1] - origin[1]) * along[1]
                if max(fa, fb) <= min(extent) + TOLERANCE or min(fa, fb) >= max(extent) - TOLERANCE:
                    continue
                if found is None or abs(position) < found[0]:
                    found = (abs(position), (a, b, m), obstruction)
        if found is None:
            return None
        # the whole straight face: the cut fill may split it where another wall joins the wall's other side
        (a, b, m), obstruction = found[1], found[2]
        length = math.hypot(b[0] - a[0], b[1] - a[1])
        u = ((b[0] - a[0]) / length, (b[1] - a[1]) / length)
        ts = [0.0, length]
        changed = True
        while changed:
            changed = False
            for c, e, n in obstruction.faces():
                if n[0] * m[0] + n[1] * m[1] < 1 - EPSILON or abs((c[0] - a[0]) * m[0] + (c[1] - a[1]) * m[1]) > TOLERANCE:
                    continue
                tc = (c[0] - a[0]) * u[0] + (c[1] - a[1]) * u[1]
                te = (e[0] - a[0]) * u[0] + (e[1] - a[1]) * u[1]
                if min(tc, te) <= max(ts) + TOLERANCE and max(tc, te) >= min(ts) - TOLERANCE \
                        and (min(tc, te) < min(ts) - EPSILON or max(tc, te) > max(ts) + EPSILON):
                    ts += [tc, te]
                    changed = True
        start, end = min(ts), max(ts)
        return ((a[0] + u[0] * start, a[1] + u[1] * start), (a[0] + u[0] * end, a[1] + u[1] * end), m), obstruction

    def _check_objects(self, objects: list[dict], obstructions: list[Obstruction]) -> list[dict]:
        rows = []
        # the outlines of the checked objects, moved as they are fixed, so a moved object does not run into another
        placed = {e["elementId"]["guid"]: e["outline"] for e in objects if e["outline"]}
        for element in objects:
            row = self._row(element)
            outline = element["outline"]
            if not outline:
                reason = element["outlineError"] or "Archicad draws no floor plan symbol for it"
                rows.append({**row, "status": "skipped", "result": f"Its outline cannot be read ({reason}); not checked."})
                continue
            floor = element["floorIndex"]
            on_floor = [o for o in obstructions if o.floorIndex == floor]
            if any(inside(centroid(outline), polygon) for o in on_floor for polygon in o.polygons):
                rows.append({**row, "status": "skipped", "result": "It is inside a wall or column; not checked."})
                continue
            margin = element["margin"]
            back = self._back_face(element, outline, on_floor)
            active = [o for o in on_floor if back is None or o.guid != back[1].guid]
            nearest = nearest_obstruction(outline, active)
            if nearest is None or nearest[0] >= margin - TOLERANCE:
                continue
            angle = float(element["details"].get("angle") or 0.0)
            row.update(obstruction=nearest[1].label, now=nearest[0], needed=margin,
                       side=side_of(outline, nearest[1], (-math.sin(angle), math.cos(angle))))

            guid = element["elementId"]["guid"]
            # another checked object counts only when it does not overlap this one already (a chair at a table)
            others = [o for g, o in placed.items() if g != guid and polygon_gap(outline, o, TOLERANCE) > -TOLERANCE]
            if back is not None:
                (a, b, _), wall = back
                length = math.hypot(b[0] - a[0], b[1] - a[1])
                direction = ((b[0] - a[0]) / length, (b[1] - a[1]) / length)
                along = [(p[0] - a[0]) * direction[0] + (p[1] - a[1]) * direction[1] for p in outline]
                t_low, t_high = -min(along), length - max(along)
                failure = f"Sliding it along {wall.label}, the wall it stands against, does not clear it within " \
                          "that wall or without running into another object."
            else:
                center = centroid(outline)
                a, b, m = min((face for face in nearest[1].faces()),
                              key=lambda f: point_segment_distance(center, f[0], f[1]))
                direction = m
                t_low, t_high = 0.0, MAX_FREE_MOVE
                failure = f"Moving it up to {fmt(MAX_FREE_MOVE)} m away from {nearest[1].label} does not clear it " \
                          "or runs into another object."
            near = self._near(outline, active, max(abs(t_low), abs(t_high)) + margin)
            near_others = [o for o in others if polygon_gap(outline, o, max(abs(t_low), abs(t_high)) + margin)
                           < max(abs(t_low), abs(t_high)) + margin]

            def clears(t):
                return self._clears(shifted(outline, direction, t), margin, near, near_others)

            shift = smallest_move(t_low, t_high, clears)
            if shift is None:
                rows.append({**row, "status": "problem", "result": failure})
                continue
            row["move"] = abs(shift)
            placed[guid] = shifted(outline, direction, shift)
            rows.append({**row, "status": "fix", "result": "",
                         "action": {"moveVector": {"x": direction[0] * shift, "y": direction[1] * shift, "z": 0.0}}})
        return rows

    @staticmethod
    def _row(element: dict) -> dict:
        return {"elementId": element["elementId"], "kind": element["kind"], "label": element["label"],
                "part": element["part"], "floorIndex": element["floorIndex"], "obstruction": "", "side": "",
                "now": None, "needed": None, "move": None, "result": ""}

    # ---- showing ----

    def _show_rows(self) -> None:
        self.tree.delete(*self.tree.get_children())
        for index, row in enumerate(self.rows):
            result = row["result"] or ("Moved." if row.get("moved") else "To move.")
            self.tree.insert("", "end", iid=str(index), tags=("moved" if row.get("moved") else row["status"],),
                             values=(row["label"], row["part"], row.get("story", ""), row["obstruction"],
                                     row["side"], fmt(row["now"]), fmt(row["needed"]), fmt(row["move"]), result))
        fix = sum(1 for r in self.rows if r["status"] == "fix")
        problems = sum(1 for r in self.rows if r["status"] == "problem")
        self.summary.configure(text=f"{fix} to move (orange), {problems} that cannot be moved (red). Distances in "
                                    "metres; Now is the smallest gap (negative: overlap). Side is seen from the "
                                    "element, facing its front. Double-click a row to select it in Archicad."
                               if self.rows else "Nothing to list.")

    def _highlight(self) -> None:
        rows = [r for r in self.rows if r["status"] in ("fix", "problem")]
        if not rows:
            run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
            return
        run_tapir("HighlightElements", {
            "elements": [{"elementId": r["elementId"]} for r in rows],
            "highlightedColors": [MOVED_COLOR if r.get("moved") else TOO_CLOSE_COLOR if r["status"] == "fix"
                                  else PROBLEM_COLOR for r in rows],
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def select_rows(self) -> None:
        rows = [self.rows[int(i)] for i in self.tree.selection()]
        if not rows:
            self.window.set_status("Pick one or more rows of the list first.", "warning")
            return
        select_elements([{"elementId": r["elementId"]} for r in rows])
        self.window.set_status(f"Selected {len(rows)} element(s) in Archicad.", "ok")

    # ---- applying ----

    def apply(self) -> None:
        settings = self._read_settings()
        if settings is None:
            return
        if self.checked_snapshot != json.dumps(self._settings_data(), sort_keys=True):
            if self.check():
                self.window.set_status("The list is updated to the current settings. Check it, then press Apply again.",
                                       "warning")
            return
        if self.applied:
            self.window.set_status("These elements are already moved. Press Check to check again.", "warning")
            return
        rows = [r for r in self.rows if r["status"] == "fix"]
        if not rows:
            self.window.set_status("There is nothing to move.", "warning")
            return
        counts = {kind: sum(1 for r in rows if r["kind"] == kind) for kind in KINDS}
        problems = sum(1 for r in self.rows if r["status"] == "problem")
        message = (f"Move {len(rows)} element(s) clear of the walls?\n\n"
                   + "\n".join(f"{KIND_LABELS[k]}: {n}" for k, n in counts.items() if n)
                   + (f"\n\n{problems} element(s) that cannot be moved are left as they are." if problems else ""))
        if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
            return

        with self.window.busy(f"Moving {len(rows)} element(s)..."):
            # one call per command, so each command is one Undo step
            for kind, command, key in (("Door", "ModifyDoors", "doorsWithDetails"),
                                       ("Window", "ModifyWindows", "windowsWithDetails"),
                                       ("Object", "MoveElements", "elementsWithMoveVectors")):
                group = [r for r in rows if r["kind"] == kind]
                if not group:
                    continue
                try:
                    results = run_tapir(command, {key: [{"elementId": r["elementId"], **r["action"]}
                                                        for r in group]}).get("executionResults", [])
                except ArchicadError as e:
                    results = [{"success": False, "error": {"message": str(e)}}] * len(group)
                results += [{"success": False, "error": {"message": "No result."}}] * (len(group) - len(results))
                for row, result in zip(group, results):
                    if isinstance(result, dict) and result.get("success", "error" not in result):
                        row["moved"] = True
                    else:
                        error = (result.get("error") or {}) if isinstance(result, dict) else {}
                        row["status"] = "problem"
                        row["result"] = f"Not moved: {error.get('message', 'unknown error')}"
        self.applied = True
        self._show_rows()
        self._highlight()
        moved = sum(1 for r in rows if r.get("moved"))
        failed = len(rows) - moved
        for row in rows:
            if not row.get("moved"):
                print(f"{row['label']}: {row['result']}")
        if failed:
            self.window.set_status(f"Moved {moved} of {len(rows)} element(s); {failed} failed (see the list).",
                                   "warning" if moved else "error")
        else:
            self.window.set_status(f"Moved {moved} element(s). They are highlighted in blue. Press Check to check again.",
                                   "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    def close(self) -> None:
        self.save_settings()
        self.clear_highlight()

    def run(self) -> None:
        self.window.run(on_start=self.load, on_close=self.close)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    WallClearance().run()
