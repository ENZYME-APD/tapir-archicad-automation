# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# changes = "model"
# ///

"""
Wall Clearance

Finds the doors, windows and objects that are too close to a wall and moves
them away just enough:
- a door or window is measured along its own wall, from each side of the
  opening to the nearest wall that ends at or crosses that wall, and is moved
  along its wall (its centerOffset, with ModifyDoors / ModifyWindows);
- an object is measured from its 2D outline (Get2DBoundingBoxes) to the
  nearest wall face, and is moved straight away from that wall (MoveElements).

The minimum distance comes, in this order, from a property picked in the
window, from a table per library part, or from the defaults (doors and windows
have one for the hinge side and one for the handle side). Check lists every
element that is too close, with the distance now, the distance needed and the
move, and the ones that cannot be moved (they would come too close to another
wall, run past the end of their wall or into another opening). They are
highlighted in Archicad. Apply moves them only after a confirmation.

The walls are taken from their floor plan cut fill (GetDetailsOfElements
floorPlanPolygons), so wall joins are as Archicad draws them. Only the walls of
the element's own story are used. The settings and the table are saved in
Documents/Tapir/WallClearance.json; the table can also be saved to and loaded
from a file of its own, for the office's standards.
"""

import fnmatch
import json
import math
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, simpledialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                run_command, run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Wall Clearance"
EPSILON = 1e-6
TOLERANCE = 0.0005
# how far from a wall's face another wall still counts as joining it
TOUCH = 0.02
# walls closer to parallel than this do not end at or cross each other
PARALLEL_DEGREES = 20.0
MAX_OBJECT_STEPS = 8

TOO_CLOSE_COLOR = [230, 140, 0, 255]
PROBLEM_COLOR = [200, 40, 40, 255]
MOVED_COLOR = [0, 130, 200, 255]
DIMMED_COLOR = [200, 200, 200, 160]

KINDS = ["Door", "Window", "Object"]
KIND_LABELS = {"Door": "Doors", "Window": "Windows", "Object": "Objects"}
DEFAULT_DISTANCES = {
    "Door": {"hinge": "0.10", "handle": "0.10"},
    "Window": {"hinge": "0.10", "handle": "0.10"},
    "Object": {"clearance": "0.20"},
}
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


def signed_area(points) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(points, points[1:] + points[:1])) / 2.0


def inside(p, points) -> bool:
    result = False
    for a, b in zip(points, points[1:] + points[:1]):
        if (a[1] > p[1]) != (b[1] > p[1]):
            x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
            if p[0] < x:
                result = not result
    return result


def clip(points, axis: int, limit: float, keep_below: bool):
    """ The part of the polygon (in local (t, s) coordinates) on one side of the line coordinate[axis] = limit. """
    def keeps(p):
        return p[axis] <= limit if keep_below else p[axis] >= limit
    result = []
    for a, b in zip(points, points[1:] + points[:1]):
        if keeps(a):
            result.append(a)
        if keeps(a) != keeps(b) and abs(b[axis] - a[axis]) > EPSILON:
            r = (limit - a[axis]) / (b[axis] - a[axis])
            result.append((a[0] + (b[0] - a[0]) * r, a[1] + (b[1] - a[1]) * r))
    return result


class Wall:
    """ A wall with its reference line (from begCoordinate to endCoordinate) as the t axis and its left normal as
    the s axis, and its body: the floor plan cut fill polygons, or a rectangle around the reference line when
    Archicad gives none. """

    def __init__(self, element_id: dict, detail: dict):
        d = detail.get("details") or {}
        self.elementId = element_id
        self.guid = element_id["guid"]
        self.floorIndex = int(detail.get("floorIndex", 0))
        self.label = detail.get("id") or "wall"
        self.beg = _xy(d["begCoordinate"])
        end = _xy(d["endCoordinate"])
        self.length = math.hypot(end[0] - self.beg[0], end[1] - self.beg[1])
        self.u = ((end[0] - self.beg[0]) / self.length, (end[1] - self.beg[1]) / self.length) \
            if self.length > EPSILON else (1.0, 0.0)
        self.n = (-self.u[1], self.u[0])
        self.curved = d.get("geometryType") == "Straight" and abs(float(d.get("arcAngle") or 0.0)) > EPSILON
        self.polygons = [p for p in (read_polygon(item.get("coordinates")) for item in detail.get("floorPlanPolygons") or [])
                         if len(p) > 2]
        self.approximate = not self.polygons
        if self.approximate and self.length > EPSILON:
            thickness = max(float(d.get("begThickness") or 0.0), float(d.get("endThickness") or 0.0)) or 0.3
            centered = d.get("referenceLineLocation") in ("Center", "CoreCenter")
            low, high = (-thickness / 2, thickness / 2) if centered else (-thickness, thickness)
            self.polygons = [[self.world(0.0, low), self.world(self.length, low),
                              self.world(self.length, high), self.world(0.0, high)]]
        local = [self.local(p) for polygon in self.polygons for p in polygon]
        if local:
            self.t_range = (min(p[0] for p in local), max(p[0] for p in local))
            self.s_range = (min(p[1] for p in local), max(p[1] for p in local))
        else:
            self.t_range, self.s_range = (0.0, self.length), (0.0, 0.0)

    def local(self, p) -> tuple[float, float]:
        dx, dy = p[0] - self.beg[0], p[1] - self.beg[1]
        return dx * self.u[0] + dy * self.u[1], dx * self.n[0] + dy * self.n[1]

    def world(self, t: float, s: float) -> tuple[float, float]:
        return (self.beg[0] + self.u[0] * t + self.n[0] * s, self.beg[1] + self.u[1] * t + self.n[1] * s)

    def faces(self):
        """ (a, b, outward unit normal) for each edge of the body. """
        for polygon in self.polygons:
            turn = 1.0 if signed_area(polygon) > 0 else -1.0
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                length = math.hypot(b[0] - a[0], b[1] - a[1])
                if length > EPSILON:
                    yield a, b, (turn * (b[1] - a[1]) / length, -turn * (b[0] - a[0]) / length)


def obstacles_along(owner: Wall, walls: list[Wall]) -> list[dict]:
    """ The stretches of the owner wall (as t intervals) that other walls of its story take where they end at it or
    cross it. A wall touching its face counts; walls nearly parallel to it do not. """
    s_low, s_high = owner.s_range[0] - TOUCH, owner.s_range[1] + TOUCH
    t_low, t_high = owner.t_range[0] - TOUCH, owner.t_range[1] + TOUCH
    parallel = math.cos(math.radians(PARALLEL_DEGREES))
    result = []
    for wall in walls:
        if wall.guid == owner.guid or wall.floorIndex != owner.floorIndex:
            continue
        if wall.length > EPSILON and abs(wall.u[0] * owner.u[0] + wall.u[1] * owner.u[1]) > parallel:
            continue
        ts = []
        for polygon in wall.polygons:
            part = [owner.local(p) for p in polygon]
            for axis, limit, below in ((1, s_high, True), (1, s_low, False), (0, t_high, True), (0, t_low, False)):
                part = clip(part, axis, limit, below)
                if not part:
                    break
            ts += [p[0] for p in part]
        if ts:
            result.append({"start": min(ts), "end": max(ts), "wall": wall})
    return result


def box_corners(box: dict) -> list[tuple[float, float]]:
    return [(box["xMin"], box["yMin"]), (box["xMax"], box["yMin"]), (box["xMax"], box["yMax"]), (box["xMin"], box["yMax"])]


def face_distance(corners, face) -> float | None:
    """ The distance of the outline from the face, when it is in front of it (its centre is on the face's outer
    side and it overlaps the face along the face); None otherwise. Negative when it reaches into the wall. """
    a, b, m = face
    length = math.hypot(b[0] - a[0], b[1] - a[1])
    ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
    along = [(c[0] - a[0]) * ux + (c[1] - a[1]) * uy for c in corners]
    if max(along) <= TOLERANCE or min(along) >= length - TOLERANCE:
        return None
    cx = sum(c[0] for c in corners) / len(corners)
    cy = sum(c[1] for c in corners) / len(corners)
    if (cx - a[0]) * m[0] + (cy - a[1]) * m[1] <= 0:
        return None
    return min((c[0] - a[0]) * m[0] + (c[1] - a[1]) * m[1] for c in corners)


class WallClearance:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds the doors, windows and objects that are too close to a wall and moves them away just enough. "
            "Set the distances, press Check to see the list, then Apply.",
            min_width=900, min_height=780)
        self.properties: dict[str, dict] = {}
        self.table: list[dict] = []
        self.rows: list[dict] = []
        self.checked_snapshot = None
        self.applied = False
        self.distance_vars: dict[str, dict[str, tk.StringVar]] = {}
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

        distances = ttk.LabelFrame(body, text="Minimum distances (m)", padding=(10, 6))
        distances.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        distances.columnconfigure(5, weight=1)
        fields = [("Door", "hinge", "Hinge side:"), ("Door", "handle", "Handle side:"),
                  ("Window", "hinge", "Hinge side:"), ("Window", "handle", "Handle side:"),
                  ("Object", "clearance", "From the nearest wall:")]
        for row, kind in enumerate(KINDS):
            ttk.Label(distances, text=KIND_LABELS[kind] + ":").grid(row=row, column=0, sticky="w", pady=(2, 0))
            self.distance_vars[kind] = {}
            column = 1
            for field_kind, key, label in fields:
                if field_kind != kind:
                    continue
                ttk.Label(distances, text=label).grid(row=row, column=column, sticky="w", padx=(10, 0), pady=(2, 0))
                var = tk.StringVar(value=DEFAULT_DISTANCES[kind][key])
                ttk.Entry(distances, textvariable=var, width=8).grid(row=row, column=column + 1, sticky="w",
                                                                     padx=(6, 0), pady=(2, 0))
                self.distance_vars[kind][key] = var
                column += 2
        self.swap_hinge = tk.BooleanVar(value=False)
        ttk.Checkbutton(distances, text="The hinge is at the other end of the opening (swap the hinge and handle sides)",
                        variable=self.swap_hinge).grid(row=3, column=0, columnspan=6, sticky="w", pady=(6, 0))
        ttk.Label(distances, text="Take the distance from a property:").grid(row=4, column=0, columnspan=2,
                                                                            sticky="w", pady=(6, 0))
        self.property = tk.StringVar(value=NO_PROPERTY)
        self.property_box = ttk.Combobox(distances, textvariable=self.property, values=[NO_PROPERTY], state="readonly",
                                         width=50)
        self.property_box.grid(row=4, column=2, columnspan=4, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(distances, text="An element whose property has a number uses it on every side, before the table "
                                  "and the defaults. Length values are in metres.", style="Muted.TLabel").grid(
            row=5, column=0, columnspan=6, sticky="w")

        table = ttk.LabelFrame(body, text="Per library part (the first matching row wins; * matches any text)",
                               padding=(10, 6))
        table.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        table.columnconfigure(0, weight=1)
        self.table_tree = make_tree(table, [("pattern", "Library part name", 380, "w"),
                                            ("first", "Hinge side, or object", 150, "e"),
                                            ("second", "Handle side", 110, "e")], height=4)
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
        self.tree = make_tree(results, [("element", "Element", 150, "w"), ("part", "Library part", 170, "w"),
                                        ("story", "Story", 80, "w"), ("wall", "Too close to", 160, "w"),
                                        ("now", "Now", 60, "e"), ("needed", "Needed", 60, "e"),
                                        ("move", "Move", 70, "e"), ("result", "Result", 220, "w")], height=10)
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
            self.table_tree.insert("", "end", iid=str(index),
                                   values=(row["pattern"], fmt(row["first"]), fmt(row.get("second"))))

    def _ask_row(self, row: dict | None) -> dict | None:
        root = self.window.root
        pattern = simpledialog.askstring(SCRIPT_NAME, "Library part name (* and ? match any text, e.g. *WC*):",
                                         initialvalue=row["pattern"] if row else "", parent=root)
        if not pattern or not pattern.strip():
            return None
        first_text = simpledialog.askstring(SCRIPT_NAME, "Hinge side of a door or window, or the distance of an "
                                                         "object from the walls (m):",
                                            initialvalue=fmt(row["first"]) if row else "", parent=root)
        if first_text is None:
            return None
        first = parse_length(first_text)
        if first is None or first < 0:
            self.window.set_status(f"'{first_text}' is not a distance in metres.", "warning")
            return None
        second_text = simpledialog.askstring(SCRIPT_NAME, "Handle side of a door or window (m); leave it empty to use "
                                                          "the same as the hinge side:",
                                             initialvalue=fmt(row.get("second")) if row else "", parent=root)
        if second_text is None:
            return None
        second = parse_length(second_text) if second_text.strip() else None
        if second_text.strip() and (second is None or second < 0):
            self.window.set_status(f"'{second_text}' is not a distance in metres.", "warning")
            return None
        return {"pattern": pattern.strip(), "first": first, "second": second}

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
            first = parse_length(item.get("first"))
            second = parse_length(item.get("second")) if item.get("second") not in (None, "") else None
            if first is not None and first >= 0:
                rows.append({"pattern": str(item["pattern"]).strip(), "first": first,
                             "second": second if second is not None and second >= 0 else None})
        return rows

    def load_table(self) -> None:
        path = filedialog.askopenfilename(parent=self.window.root, title="Load the distances",
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
        self._apply_distances(data.get("defaults"))
        self._list_table()
        self.window.set_status(f"Loaded {len(self.table)} row(s) and the default distances from {path}.", "ok")

    def save_table(self) -> None:
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the distances",
                                            defaultextension=".json", initialfile="Wall Clearance.json",
                                            filetypes=[("Wall Clearance table", "*.json")])
        if not path:
            return
        data = {"defaults": self._distance_texts(), "table": self.table}
        try:
            with open(path, "w", encoding="utf-8") as file:
                json.dump(data, file, indent=2, ensure_ascii=False)
        except OSError as e:
            self.window.set_status(f"Cannot save {path}: {e}", "error")
            return
        self.window.set_status(f"Saved the default distances and {len(self.table)} row(s) to {path}.", "ok")

    # ---- settings ----

    def _distance_texts(self) -> dict:
        return {kind: {key: var.get() for key, var in fields.items()} for kind, fields in self.distance_vars.items()}

    def _apply_distances(self, data) -> None:
        if not isinstance(data, dict):
            return
        for kind, fields in self.distance_vars.items():
            values = data.get(kind)
            if isinstance(values, dict):
                for key, var in fields.items():
                    if values.get(key) is not None:
                        var.set(str(values[key]))

    def _settings_data(self) -> dict:
        prop = self.properties.get(self.property.get())
        return {
            "scope": self.scope.get(),
            "kinds": {kind: var.get() for kind, var in self.kind_vars.items()},
            "partFilter": self.part_filter.get(),
            "defaults": self._distance_texts(),
            "swapHinge": self.swap_hinge.get(),
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
        self._apply_distances(data.get("defaults"))
        self.swap_hinge.set(bool(data.get("swapHinge")))
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
        defaults = {}
        for kind, fields in self.distance_vars.items():
            defaults[kind] = {}
            for key, var in fields.items():
                value = parse_length(var.get())
                if value is None or value < 0:
                    self.window.set_status(f"The {key} distance of the {KIND_LABELS[kind].lower()} must be a "
                                           "non-negative number of metres.", "warning")
                    return None
                defaults[kind][key] = value
        prop = self.properties.get(self.property.get())
        return {"scope": self.scope.get(), "kinds": kinds, "partFilter": self.part_filter.get().lower().split(),
                "defaults": defaults, "swapHinge": self.swap_hinge.get(),
                "property": prop["propertyId"] if prop else None,
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
        self.window.set_status("Set the distances and press Check.")

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

    def _read_walls(self, floors: set[int]) -> list[Wall]:
        wall_ids = run_tapir("GetElementsByType", {"elementType": "Wall"}).get("elements", [])
        floor_details = get_details_of_elements(wall_ids, ["floorIndex"])
        wall_ids = [w for w, d in zip(wall_ids, floor_details) if int(d.get("floorIndex", 0)) in floors]
        walls = []
        details = get_details_of_elements(wall_ids, ["type", "id", "floorIndex", "details", "floorPlanPolygons"])
        for wall_id, detail in zip(wall_ids, details):
            d = detail.get("details") or {}
            if detail.get("type") == "Wall" and "begCoordinate" in d and "endCoordinate" in d:
                walls.append(Wall(wall_id["elementId"], detail))
        return walls

    def _distance_for(self, kind: str, part: str, value: float | None, settings: dict) -> tuple[float, float]:
        """ (hinge side, handle side) of a door or window, (distance, distance) of an object. """
        if value is not None:
            return value, value
        name = part.lower()
        for row in settings["table"]:
            if fnmatch.fnmatchcase(name, row["pattern"].lower()):
                second = row["second"] if row.get("second") is not None and kind != "Object" else row["first"]
                return row["first"], second
        defaults = settings["defaults"][kind]
        if kind == "Object":
            return defaults["clearance"], defaults["clearance"]
        return defaults["hinge"], defaults["handle"]

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
                element["value"] = value
            self.window.set_status("Reading the walls...")
            walls = self._read_walls({e["floorIndex"] for e in elements})
            stories = get_story_names()
            openings = [e for e in elements if e["kind"] in ("Door", "Window")]
            objects = [e for e in elements if e["kind"] == "Object"]
            rows = []
            if openings:
                self.window.set_status("Measuring the doors and windows...")
                rows += self._check_openings(openings, walls, settings)
            if objects:
                self.window.set_status("Measuring the objects...")
                rows += self._check_objects(objects, walls, settings)
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
        approximate = any(w.approximate for w in walls)
        message = (f"Checked {len(elements)} element(s): {fix} to move, {problems} that cannot be moved, "
                   f"{skipped} not checked, {len(elements) - fix - problems - skipped} far enough from the walls.")
        if approximate:
            message += " Some walls have no floor plan outline; they are measured from their reference line."
        self.window.set_status(message, "warning" if fix or problems else "ok")
        return True

    def _check_openings(self, openings: list[dict], walls: list[Wall], settings: dict) -> list[dict]:
        wall_by_guid = {w.guid: w for w in walls}
        # every door and window in the owner walls, so a moved one does not run into another
        positions: dict[str, dict] = {}
        owners = {(e["details"].get("ownerElementId") or {}).get("guid") for e in openings}
        for kind in ("Door", "Window"):
            try:
                ids = run_tapir("GetElementsByType", {"elementType": kind}).get("elements", [])
            except ArchicadError as e:
                print(f"Cannot read the {kind.lower()}s: {e}")
                continue
            ids_and_details = zip(ids, get_details_of_elements(ids, ["id", "details"]))
            for element_id, detail in ids_and_details:
                d = detail.get("details") or {}
                owner = (d.get("ownerElementId") or {}).get("guid")
                if owner in owners and "centerOffset" in d:
                    half = float(d.get("width", 0.0)) / 2.0
                    positions[element_id["elementId"]["guid"]] = {
                        "owner": owner, "label": detail.get("id") or kind.lower(),
                        "low": float(d["centerOffset"]) - half, "high": float(d["centerOffset"]) + half}

        rows = []
        obstacles_of: dict[str, list] = {}
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
            if owner.guid not in obstacles_of:
                obstacles_of[owner.guid] = obstacles_along(owner, walls)
            obstacles = obstacles_of[owner.guid]
            center, half = float(d["centerOffset"]), float(d.get("width", 0.0)) / 2.0
            low, high = center - half, center + half
            hinge, handle = self._distance_for(element["kind"], element["part"], element["value"], settings)
            hinge_at_begin = (bool(d.get("reflected")) == bool(d.get("oSide"))) != settings["swapHinge"]
            need_begin, need_end = (hinge, handle) if hinge_at_begin else (handle, hinge)
            name_begin, name_end = ("hinge side", "handle side") if hinge_at_begin else ("handle side", "hinge side")

            crossing = [o for o in obstacles if o["end"] > low + TOLERANCE and o["start"] < high - TOLERANCE]
            if crossing:
                rows.append({**row, "status": "problem", "wall": crossing[0]["wall"].label,
                             "result": "A wall crosses the opening."})
                continue
            before = [o for o in obstacles if o["end"] <= low + TOLERANCE]
            after = [o for o in obstacles if o["start"] >= high - TOLERANCE]
            near_begin = max(before, key=lambda o: o["end"]) if before else None
            near_end = min(after, key=lambda o: o["start"]) if after else None
            gap_begin = low - near_begin["end"] if near_begin else None
            gap_end = near_end["start"] - high if near_end else None
            short_begin = gap_begin is not None and gap_begin < need_begin - TOLERANCE
            short_end = gap_end is not None and gap_end < need_end - TOLERANCE
            if not short_begin and not short_end:
                continue

            if short_begin and short_end:
                rows.append({**row, "status": "problem", "wall": f"{near_begin['wall'].label}, {near_end['wall'].label}",
                             "now": min(gap_begin, gap_end), "needed": max(need_begin, need_end),
                             "result": f"Too close on both sides: {fmt(gap_begin)} m on the {name_begin}, "
                                       f"{fmt(gap_end)} m on the {name_end}."})
                continue
            if short_begin:
                shift = need_begin - gap_begin
                row.update(wall=f"{near_begin['wall'].label} ({name_begin})", now=gap_begin, needed=need_begin)
                other_gap, other_need, other_name = gap_end, need_end, name_end
            else:
                shift = -(need_end - gap_end)
                row.update(wall=f"{near_end['wall'].label} ({name_end})", now=gap_end, needed=need_end)
                other_gap, other_need, other_name = gap_begin, need_begin, name_begin
            row["move"] = abs(shift)
            new_low, new_high = low + shift, high + shift
            guid = element["elementId"]["guid"]
            blocked_by = next((p for g, p in positions.items() if g != guid and p["owner"] == owner.guid
                               and p["high"] > new_low + TOLERANCE and p["low"] < new_high - TOLERANCE), None)
            if other_gap is not None and other_gap - abs(shift) < other_need - TOLERANCE:
                result = (f"Not enough room between the walls: moving it {fmt(abs(shift))} m leaves "
                          f"{fmt(other_gap - abs(shift))} m on the {other_name}, {fmt(other_need)} m needed.")
                rows.append({**row, "status": "problem", "result": result})
            elif (new_low < owner.t_range[0] - TOLERANCE or new_high > owner.t_range[1] + TOLERANCE
                  or center + shift < 0):
                rows.append({**row, "status": "problem", "result": "Moving it would take it past the end of its wall."})
            elif blocked_by is not None:
                rows.append({**row, "status": "problem", "result": f"Moving it would run into {blocked_by['label']}."})
            else:
                if guid in positions:
                    positions[guid].update(low=new_low, high=new_high)
                rows.append({**row, "status": "fix", "result": "", "action": {"centerOffset": center + shift}})
        return rows

    def _check_objects(self, objects: list[dict], walls: list[Wall], settings: dict) -> list[dict]:
        boxes = []
        for chunk in batched([{"elementId": e["elementId"]} for e in objects]):
            result = run_command("Get2DBoundingBoxes", {"elements": chunk}).get("boundingBoxes2D", [])
            boxes += [item.get("boundingBox2D") if isinstance(item, dict) else None for item in result]
        boxes += [None] * (len(objects) - len(boxes))
        faces_of: dict[int, list] = {}
        rows = []
        for element, box in zip(objects, boxes):
            row = self._row(element)
            if not box:
                rows.append({**row, "status": "skipped", "result": "Its 2D outline cannot be read; not checked."})
                continue
            floor = element["floorIndex"]
            if floor not in faces_of:
                faces_of[floor] = [(face, wall) for wall in walls if wall.floorIndex == floor for face in wall.faces()]
            faces = faces_of[floor]
            corners = box_corners(box)
            center = ((box["xMin"] + box["xMax"]) / 2, (box["yMin"] + box["yMax"]) / 2)
            if any(inside(center, polygon) for wall in walls if wall.floorIndex == floor for polygon in wall.polygons):
                rows.append({**row, "status": "skipped", "result": "It is inside a wall; not checked."})
                continue
            need = self._distance_for("Object", element["part"], element["value"], settings)[0]

            def worst(points):
                found = None
                for face, wall in faces:
                    distance = face_distance(points, face)
                    if distance is not None and distance < need - TOLERANCE and (found is None or distance < found[0]):
                        found = (distance, face, wall)
                return found

            first = worst(corners)
            if first is None:
                continue
            row.update(wall=first[2].label, now=first[0], needed=need)
            dx = dy = 0.0
            remaining = first
            for _ in range(MAX_OBJECT_STEPS):
                if remaining is None:
                    break
                distance, (_, _, m), _ = remaining
                dx += m[0] * (need - distance)
                dy += m[1] * (need - distance)
                remaining = worst([(x + dx, y + dy) for x, y in corners])
            row["move"] = math.hypot(dx, dy)
            if remaining is not None:
                rows.append({**row, "status": "problem",
                             "result": f"Not enough room: it cannot keep {fmt(need)} m from {remaining[2].label} "
                                       "and the other walls around it."})
            else:
                rows.append({**row, "status": "fix", "result": "", "action": {"moveVector": {"x": dx, "y": dy, "z": 0.0}}})
        return rows

    @staticmethod
    def _row(element: dict) -> dict:
        return {"elementId": element["elementId"], "kind": element["kind"], "label": element["label"],
                "part": element["part"], "floorIndex": element["floorIndex"], "wall": "", "now": None,
                "needed": None, "move": None, "result": ""}

    # ---- showing ----

    def _show_rows(self) -> None:
        self.tree.delete(*self.tree.get_children())
        for index, row in enumerate(self.rows):
            result = row["result"] or ("Moved." if row.get("moved") else "To move.")
            self.tree.insert("", "end", iid=str(index), tags=("moved" if row.get("moved") else row["status"],),
                             values=(row["label"], row["part"], row.get("story", ""), row["wall"], fmt(row["now"]),
                                     fmt(row["needed"]), fmt(row["move"]), result))
        fix = sum(1 for r in self.rows if r["status"] == "fix")
        problems = sum(1 for r in self.rows if r["status"] == "problem")
        self.summary.configure(text=f"{fix} to move (orange), {problems} that cannot be moved (red). Distances in "
                                    "metres. Double-click a row to select it in Archicad." if self.rows
                               else "Nothing to list.")

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
        message = (f"Move {len(rows)} element(s) away from the walls?\n\n"
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
