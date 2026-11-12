# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "modeling-geometry"
# changes = "model"
# ///

"""
Place Symbols in Rooms

Places a library part in the chosen zones by a rule:
- along the zone's boundary at a spacing and a mounting height, keeping a
  distance from the corners and the doors (for example sockets), or
- in a grid with a maximum spacing and a margin from the walls (for example
  lights).

Each new element gets the layer and the GDL parameter values set in the window.
Preview first lists the number of elements per zone, draws their positions and
highlights the zones in Archicad; Place creates them only after a confirmation.

The zone's polygon is used as its boundary (its curved edges get no elements
along them). Doors are found with GetRelationsOfElements (Archicad 26 or later).
"""

import math
import tkinter as tk
from tkinter import simpledialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Place Symbols in Rooms"
EPSILON = 1e-6
DIMMED_COLOR = [200, 200, 200, 160]
WITH_POINTS_COLOR = [27, 127, 59, 255]
WITHOUT_POINTS_COLOR = [230, 140, 0, 255]
CREATED_COLOR = [0, 130, 200, 255]
NO_LAYER = "(the tool's default layer)"
MAX_LISTED_PARTS = 500
DEFAULT_WALL_THICKNESS = 0.5
INTEGER_TYPES = {"Integer", "PenColor", "LineType", "Material", "FillPattern", "BuildingMaterial", "Profile"}
REAL_TYPES = {"Length", "Angle", "RealNumber", "Intensity"}
EDITABLE_TYPES = INTEGER_TYPES | REAL_TYPES | {"Boolean", "LightSwitch", "String"}

BOUNDARY_FIELDS = [
    ("spacing", "Spacing (m):", "3.0"),
    ("height", "Mounting height (m):", "0.3"),
    ("corner", "Distance from corners (m):", "0.3"),
    ("door", "Distance from doors (m):", "0.3"),
]
GRID_FIELDS = [
    ("spacing", "Maximum spacing (m):", "2.4"),
    ("height", "Mounting height (m):", "2.6"),
    ("margin", "Margin from walls (m):", "0.6"),
]


# ---- geometry ----

def _xy(point: dict) -> tuple[float, float]:
    return float(point["x"]), float(point["y"])


def _same(a, b) -> bool:
    return abs(a[0] - b[0]) < EPSILON and abs(a[1] - b[1]) < EPSILON


def read_contour(points: list[dict], arcs: list[dict] | None) -> dict:
    """ {"points": [(x, y)], "curved": {edge index}}; the repeated closing point is left out, so
    edge i runs from point i to point i + 1 (the last one back to the first). """
    pts = [_xy(p) for p in points or []]
    if len(pts) > 1 and _same(pts[0], pts[-1]):
        pts.pop()
    curved = {int(a.get("begIndex", -1)) % max(1, len(pts)) for a in arcs or []
              if abs(float(a.get("arcAngle", 0.0))) > EPSILON}
    return {"points": pts, "curved": curved}


def edges(points: list[tuple[float, float]]):
    for i, a in enumerate(points):
        yield i, a, points[(i + 1) % len(points)]


def signed_area(points) -> float:
    return sum(a[0] * b[1] - b[0] * a[1] for _, a, b in edges(points)) / 2.0


def centroid(points) -> tuple[float, float]:
    area = signed_area(points)
    if abs(area) < EPSILON:
        return sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points)
    cx = cy = 0.0
    for _, a, b in edges(points):
        cross = a[0] * b[1] - b[0] * a[1]
        cx += (a[0] + b[0]) * cross
        cy += (a[1] + b[1]) * cross
    return cx / (6.0 * area), cy / (6.0 * area)


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


def subtract_intervals(start: float, end: float, cuts: list[tuple[float, float]]) -> list[tuple[float, float]]:
    """ The parts of [start, end] that no cut covers. """
    parts = [(start, end)] if end >= start - EPSILON else []
    for cut_start, cut_end in cuts:
        remaining = []
        for s, e in parts:
            if cut_end <= s or cut_start >= e:
                remaining.append((s, e))
                continue
            if cut_start > s:
                remaining.append((s, cut_start))
            if cut_end < e:
                remaining.append((cut_end, e))
        parts = remaining
    return parts


def positions_at_spacing(start: float, end: float, spacing: float) -> list[float]:
    """ Positions exactly spacing apart in [start, end], centered in it. """
    length = max(0.0, end - start)
    count = int(math.floor(length / spacing + EPSILON)) + 1
    first = start + (length - (count - 1) * spacing) / 2.0
    return [first + i * spacing for i in range(count)]


def door_cuts(outline: list[tuple[float, float]], doors: list[dict], clearance: float) -> dict[int, list]:
    """ Per edge of the outline, the stretches (distances from the edge's start) a door with its
    clearance takes. A door belongs to the nearest edge its wall is thick enough to reach. """
    cuts: dict[int, list] = {}
    for door in doors:
        center, half = door["center"], door["width"] / 2.0
        best = None
        for i, a, b in edges(outline):
            length = math.hypot(b[0] - a[0], b[1] - a[1])
            if length < EPSILON:
                continue
            ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
            t = (center[0] - a[0]) * ux + (center[1] - a[1]) * uy
            d = abs((center[0] - a[0]) * uy - (center[1] - a[1]) * ux)
            if -half <= t <= length + half and d <= door["reach"] and (best is None or d < best[0]):
                best = (d, i, t)
        if best is not None:
            _, i, t = best
            cuts.setdefault(i, []).append((t - half - clearance, t + half + clearance))
    return cuts


def boundary_points(outline: dict, spacing: float, corner: float, cuts: dict[int, list]) -> list[tuple]:
    """ (x, y, angle) along the straight edges of the outline. The angle turns the element's
    +y side towards the inside of the zone. """
    points = outline["points"]
    turn = 0.0 if signed_area(points) > 0 else math.pi
    result = []
    for i, a, b in edges(points):
        if i in outline["curved"]:
            continue
        length = math.hypot(b[0] - a[0], b[1] - a[1])
        if length < EPSILON or length < 2 * corner:
            continue
        ux, uy = (b[0] - a[0]) / length, (b[1] - a[1]) / length
        angle = (math.atan2(uy, ux) + turn) % (2 * math.pi)
        for start, end in subtract_intervals(corner, length - corner, cuts.get(i, [])):
            for t in positions_at_spacing(start, end, spacing):
                result.append((a[0] + ux * t, a[1] + uy * t, angle))
    return result


def _axis_positions(low: float, high: float, spacing: float) -> list[float]:
    """ From low to high, evenly, at most spacing apart; the middle when there is no room. """
    width = high - low
    if width <= EPSILON:
        return [(low + high) / 2.0]
    count = int(math.ceil(width / spacing - EPSILON)) + 1
    return [low + i * width / (count - 1) for i in range(count)]


def grid_points(outline: dict, holes: list[dict], spacing: float, margin: float) -> list[tuple]:
    """ (x, y, 0) in a grid over the zone, at least margin from its edges. """
    points = outline["points"]
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    contours = [points] + [h["points"] for h in holes if len(h["points"]) > 2]

    def fits(p) -> bool:
        if not inside(p, points) or any(inside(p, h) for h in contours[1:]):
            return False
        return all(distance_to_segment(p, a, b) >= margin - EPSILON for c in contours for _, a, b in edges(c))

    result = [(x, y, 0.0)
              for y in _axis_positions(min(ys) + margin, max(ys) - margin, spacing)
              for x in _axis_positions(min(xs) + margin, max(xs) - margin, spacing)
              if fits((x, y))]
    if not result:
        center = centroid(points)
        if inside(center, points):
            result.append((center[0], center[1], 0.0))
    return result


def parse_length(text: str) -> float | None:
    try:
        return float(text.strip().replace(",", "."))
    except ValueError:
        return None


def parse_parameter_value(text: str, parameter_type: str):
    """ The typed text as the value SetGDLParametersOfElements takes for the type; None if invalid. """
    text = text.strip()
    if parameter_type == "Boolean":
        lowered = text.lower()
        if lowered in ("1", "true", "yes", "on"):
            return True
        if lowered in ("0", "false", "no", "off"):
            return False
        return None
    if parameter_type == "LightSwitch":
        lowered = text.lower()
        if lowered in ("1", "true", "yes", "on"):
            return "On"
        if lowered in ("0", "false", "no", "off"):
            return "Off"
        return None
    if parameter_type in INTEGER_TYPES:
        try:
            return int(text)
        except ValueError:
            return None
    if parameter_type in REAL_TYPES:
        return parse_length(text)
    return text


class PlaceSymbolsInRooms:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Places a library part in the zones: along the boundary (e.g. sockets) or in a grid (e.g. lights). "
            "Pick the part and the rule, press Preview to see the positions, then Place.",
            min_width=820, min_height=760)
        self.library_parts: list[dict] = []
        self.listed_parts: list[dict] = []
        self.part: dict | None = None
        self.parameters: list[dict] = []
        self.parameter_values: dict[str, object] = {}
        self.layers: dict[str, int] = {}
        # the last preview: {"settings", "zones": [{"elementId", "label", "floorIndex", "z", "outline", "holes",
        # "doors", "points", "note"}]}
        self.preview: dict | None = None
        self.created: list[dict] = []
        self.field_vars: dict[str, dict[str, tk.StringVar]] = {"boundary": {}, "grid": {}}
        self._build_body()
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Place", self.place)
        self.window.add_button("Preview", self.make_preview, primary=True)

    # ---- window ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)
        body.rowconfigure(4, weight=2)

        zones = ttk.LabelFrame(body, text="Zones", padding=(10, 6))
        zones.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="selection")
        for column, (value, text) in enumerate([("selection", "Selected zones"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(zones, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))

        parts = ttk.LabelFrame(body, text="Library part", padding=(10, 6))
        parts.grid(row=1, column=0, sticky="nsew", pady=(8, 0))
        parts.columnconfigure(1, weight=1)
        parts.rowconfigure(1, weight=1)
        ttk.Label(parts, text="Search:").grid(row=0, column=0, sticky="w")
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_: self._list_parts())
        ttk.Entry(parts, textvariable=self.search).grid(row=0, column=1, sticky="ew", padx=(6, 0))
        self.part_tree = make_tree(parts, [("name", "Name", 420, "w"), ("type", "Type", 80, "w")], height=5)
        self.part_tree.configure(selectmode="browse")
        self.part_tree.container.grid(row=1, column=0, columnspan=2, sticky="nsew", pady=(6, 0))
        self.part_tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._pick_part)())

        rule = ttk.LabelFrame(body, text="Rule", padding=(10, 6))
        rule.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        self.mode = tk.StringVar(value="boundary")
        for column, (mode, text, fields) in enumerate([("boundary", "Along the boundary", BOUNDARY_FIELDS),
                                                       ("grid", "In a grid", GRID_FIELDS)]):
            frame = ttk.Frame(rule)
            frame.grid(row=0, column=column, sticky="nw", padx=(0 if column == 0 else 24, 0))
            ttk.Radiobutton(frame, text=text, value=mode, variable=self.mode).grid(
                row=0, column=0, columnspan=2, sticky="w")
            for row, (key, label, default) in enumerate(fields, start=1):
                ttk.Label(frame, text=label).grid(row=row, column=0, sticky="w", padx=(20, 0), pady=(3, 0))
                var = tk.StringVar(value=default)
                ttk.Entry(frame, textvariable=var, width=8).grid(row=row, column=1, sticky="w", padx=(6, 0), pady=(3, 0))
                self.field_vars[mode][key] = var

        new = ttk.LabelFrame(body, text="New elements", padding=(10, 6))
        new.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        new.columnconfigure(1, weight=1)
        ttk.Label(new, text="Layer:").grid(row=0, column=0, sticky="w")
        self.layer = tk.StringVar(value=NO_LAYER)
        self.layer_box = ttk.Combobox(new, textvariable=self.layer, values=[NO_LAYER], state="readonly", width=40)
        self.layer_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(new, text="Parameters (double-click a value to change it; only changed values are written):",
                  style="Muted.TLabel").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.parameter_tree = make_tree(new, [("description", "Parameter", 260, "w"), ("name", "Name", 120, "w"),
                                              ("type", "Type", 90, "w"), ("value", "Value", 160, "w")], height=5)
        self.parameter_tree.configure(selectmode="browse")
        self.parameter_tree.container.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(4, 0))
        self.parameter_tree.bind("<Double-1>", lambda _e: self.window.guarded(self._edit_parameter)())

        preview = ttk.Frame(body, padding=(0, 8, 0, 0))
        preview.grid(row=4, column=0, sticky="nsew")
        preview.columnconfigure(0, weight=1)
        preview.columnconfigure(1, weight=1)
        preview.rowconfigure(1, weight=1)
        self.summary_label = ttk.Label(preview, text="Pick a library part and press Preview.")
        self.summary_label.grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 4))
        self.zone_tree = make_tree(preview, [("zone", "Zone", 200, "w"), ("story", "Story", 100, "w"),
                                             ("count", "Elements", 70, "e"), ("note", "Note", 120, "w")], height=8)
        self.zone_tree.container.grid(row=1, column=0, sticky="nsew")
        self.zone_tree.bind("<<TreeviewSelect>>", lambda _e: self._draw())
        self.zone_tree.bind("<Double-1>", lambda _e: self.window.guarded(self._select_zone)())
        self.canvas = tk.Canvas(preview, background="white", highlightthickness=1, highlightbackground="#bbbbbb",
                                width=320, height=220)
        self.canvas.grid(row=1, column=1, sticky="nsew", padx=(8, 0))
        self.canvas.bind("<Configure>", lambda _e: self._draw())

    # ---- start ----

    def load(self) -> None:
        with self.window.busy("Reading the library parts and the layers..."):
            parts = []
            for type_id in ("Object", "Lamp"):
                try:
                    found = run_tapir("GetAvailableLibraryParts", {"filterByTypeId": type_id}).get("libraryParts", [])
                except ArchicadError as e:
                    print(f"Skipping the {type_id} library parts: {e}")
                    continue
                parts += [{"name": p["documentName"], "typeId": type_id} for p in found if p.get("documentName")]
            seen = set()
            self.library_parts = []
            for part in sorted(parts, key=lambda p: p["name"].lower()):
                if part["name"] not in seen:
                    seen.add(part["name"])
                    self.library_parts.append(part)
            layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
            self.layers = {}
            for layer in sorted(layers, key=lambda a: str(a.get("name", "")).lower()):
                label = str(layer.get("name", ""))
                if label in self.layers:
                    label = f"{label} ({layer['index']})"
                self.layers[label] = int(layer["index"])
            self.layer_box.configure(values=[NO_LAYER] + list(self.layers))
        self._list_parts()
        self.window.set_status(f"{len(self.library_parts)} library parts. Pick one, set the rule and press Preview.")

    def _list_parts(self) -> None:
        words = self.search.get().lower().split()
        matching = [p for p in self.library_parts if all(w in p["name"].lower() for w in words)]
        self.listed_parts = matching[:MAX_LISTED_PARTS]
        self.part_tree.delete(*self.part_tree.get_children())
        for index, part in enumerate(self.listed_parts):
            self.part_tree.insert("", "end", iid=str(index), values=(part["name"], part["typeId"]))
        if len(matching) > MAX_LISTED_PARTS:
            self.window.set_status(f"{len(matching)} library parts match; the first {MAX_LISTED_PARTS} are listed. "
                                   "Type more of the name to find the others.")
        if self.part is not None and self.part in self.listed_parts:
            self.part_tree.selection_set(str(self.listed_parts.index(self.part)))

    def _pick_part(self) -> None:
        rows = self.part_tree.selection()
        if not rows:
            return
        part = self.listed_parts[int(rows[0])]
        if part is self.part:
            return
        self.part = part
        self.parameter_values = {}
        with self.window.busy(f"Reading the parameters of {part['name']}..."):
            result = run_tapir("GetGDLParametersOfLibraryParts", {"libraryParts": [{"libraryPartName": part["name"]}]})
        item = (result.get("gdlParametersOfLibraryParts") or [{}])[0]
        if "error" in item:
            self.parameters = []
            self.window.set_status(f"Cannot read the parameters of {part['name']}: {item_errors([item])[0]}", "warning")
        else:
            self.parameters = [p for p in item.get("parameters", [])
                               if p.get("type") in EDITABLE_TYPES and "Hidden" not in p.get("flags", [])
                               and not p.get("dimension1") and not p.get("dimension2")]
            self.window.set_status(f"{part['name']} picked. Press Preview.")
        self._list_parameters()

    def _list_parameters(self) -> None:
        self.parameter_tree.delete(*self.parameter_tree.get_children())
        for index, p in enumerate(self.parameters):
            name = p["name"]
            value = self.parameter_values.get(name, p.get("value", ""))
            shown = f"{value}  (changed)" if name in self.parameter_values else str(value)
            self.parameter_tree.insert("", "end", iid=str(index),
                                       values=(p.get("displayName") or name, name, p.get("type", ""), shown))

    def _edit_parameter(self) -> None:
        row = self.parameter_tree.focus()
        if not row:
            return
        p = self.parameters[int(row)]
        name = p["name"]
        current = self.parameter_values.get(name, p.get("value", ""))
        text = simpledialog.askstring(
            SCRIPT_NAME, f"{p.get('displayName') or name} ({name}, {p.get('type')}).\n"
                         "Leave it empty to keep the library part's own value.",
            initialvalue=str(current), parent=self.window.root)
        if text is None:
            return
        if not text.strip() and p.get("type") != "String":
            self.parameter_values.pop(name, None)
        else:
            value = parse_parameter_value(text, p.get("type", ""))
            if value is None:
                self.window.set_status(f"'{text}' is not a valid {p.get('type')} value.", "warning")
                return
            if value == p.get("value"):
                self.parameter_values.pop(name, None)
            else:
                self.parameter_values[name] = value
        self._list_parameters()
        self.parameter_tree.selection_set(row)

    # ---- preview ----

    def _settings(self) -> dict | None:
        """ The rule's settings, or None after showing what is wrong. """
        mode = self.mode.get()
        settings = {"mode": mode, "scope": self.scope.get()}
        fields = BOUNDARY_FIELDS if mode == "boundary" else GRID_FIELDS
        for key, label, _ in fields:
            value = parse_length(self.field_vars[mode][key].get())
            if value is None or value < 0 or (key == "spacing" and value <= EPSILON):
                self.window.set_status(f"{label.rstrip(':')} must be a {'positive' if key == 'spacing' else 'non-negative'}"
                                       " number of metres.", "warning")
                return None
            settings[key] = value
        return settings

    def _read_doors(self, zones: list[dict]) -> tuple[list[list[dict]], str]:
        """ The doors of each zone as {"center", "width", "reach"}, and a warning when they cannot be read. """
        doors_of_zones: list[list[dict]] = [[] for _ in zones]
        door_ids: dict[str, int] = {}
        try:
            base = 0
            for chunk in batched(zones):
                relations = run_tapir("GetRelationsOfElements", {"elements": [{"elementId": z["elementId"]} for z in chunk]})
                for offset, relation in enumerate(relations.get("relations", [])[:len(chunk)]):
                    groups = (relation.get("zoneRelations") or {}).get("elementsGroupedByType", [])
                    for group in groups:
                        if group.get("elementType") == "Door":
                            for door in group.get("elements", []):
                                door_ids.setdefault(door["elementId"]["guid"], -1)
                                doors_of_zones[base + offset].append({"guid": door["elementId"]["guid"]})
                base += len(chunk)
        except ArchicadError as e:
            return [[] for _ in zones], f"The doors could not be read ({e}), so no distance is kept from them."
        if not door_ids:
            return doors_of_zones, ""

        guids = list(door_ids)
        door_details = get_details_of_elements([{"elementId": {"guid": g}} for g in guids], ["details"])
        doors: dict[str, dict] = {}
        walls: dict[str, None] = {}
        for guid, detail in zip(guids, door_details):
            d = detail.get("details") or {}
            owner = (d.get("ownerElementId") or {}).get("guid")
            if owner and "centerOffset" in d:
                doors[guid] = {"owner": owner, "offset": float(d["centerOffset"]), "width": float(d.get("width", 0.0))}
                walls[owner] = None
        wall_guids = list(walls)
        wall_details = get_details_of_elements([{"elementId": {"guid": g}} for g in wall_guids], ["type", "details"])
        wall_by_guid = dict(zip(wall_guids, wall_details))
        skipped = 0
        for guid, door in list(doors.items()):
            wall = wall_by_guid.get(door["owner"]) or {}
            d = wall.get("details") or {}
            if wall.get("type") != "Wall" or "begCoordinate" not in d or "endCoordinate" not in d:
                del doors[guid]
                skipped += 1
                continue
            beg, end = _xy(d["begCoordinate"]), _xy(d["endCoordinate"])
            length = math.hypot(end[0] - beg[0], end[1] - beg[1])
            if length < EPSILON:
                del doors[guid]
                skipped += 1
                continue
            t = door["offset"] / length
            door["center"] = (beg[0] + (end[0] - beg[0]) * t, beg[1] + (end[1] - beg[1]) * t)
            thickness = max(float(d.get("begThickness") or 0.0), float(d.get("endThickness") or 0.0))
            door["reach"] = (thickness if thickness > EPSILON else DEFAULT_WALL_THICKNESS) + 0.05
        for zone_doors in doors_of_zones:
            zone_doors[:] = [doors[d["guid"]] for d in zone_doors if d["guid"] in doors]
        warning = f"{skipped} door(s) not in a wall were not kept clear." if skipped else ""
        return doors_of_zones, warning

    def make_preview(self) -> bool:
        if self.part is None:
            self.window.set_status("Pick a library part first.", "warning")
            return False
        settings = self._settings()
        if settings is None:
            return False
        with self.window.busy("Reading the zones..."):
            zone_ids = get_elements(settings["scope"], ["Zone"])
            if not zone_ids:
                self.window.set_status("There are no zones in the chosen scope." if settings["scope"] != "selection"
                                       else "Select some zones in Archicad first.", "warning")
                return False
            details = get_details_of_elements(zone_ids, ["type", "floorIndex", "details"])
            stories = get_story_names()
            zones = []
            for zone_id, detail in zip(zone_ids, details):
                d = detail.get("details") or {}
                if detail.get("type") != "Zone" or "polygonOutline" not in d:
                    continue
                outline = read_contour(d["polygonOutline"], d.get("polygonArcs"))
                if len(outline["points"]) < 3:
                    continue
                floor = int(detail.get("floorIndex", 0))
                name = d.get("name", "")
                number = d.get("numberStr", "")
                zones.append({
                    "elementId": zone_id["elementId"],
                    "label": f"{number} {name}".strip() or "(no name)",
                    "story": stories.get(floor, str(floor)),
                    "floorIndex": floor,
                    "z": float(d.get("zCoordinate", 0.0)),
                    "outline": outline,
                    "holes": [read_contour(h.get("polygonOutline") or h.get("polygonCoordinates"), h.get("polygonArcs"))
                              for h in d.get("holes") or []],
                })
            warning = ""
            if settings["mode"] == "boundary":
                self.window.set_status("Reading the doors...")
                doors_of_zones, warning = self._read_doors(zones)
            for index, zone in enumerate(zones):
                if settings["mode"] == "boundary":
                    zone["doors"] = doors_of_zones[index]
                    cuts = door_cuts(zone["outline"]["points"], zone["doors"], settings["door"])
                    zone["points"] = boundary_points(zone["outline"], settings["spacing"], settings["corner"], cuts)
                    zone["note"] = "curved edges skipped" if zone["outline"]["curved"] else ""
                else:
                    zone["doors"] = []
                    zone["points"] = grid_points(zone["outline"], zone["holes"], settings["spacing"], settings["margin"])
                    zone["note"] = ""
                if not zone["points"]:
                    zone["note"] = "no room for the rule"

        zones.sort(key=lambda z: (z["floorIndex"], z["label"].lower()))
        self.preview = {"settings": self._snapshot(settings), "settingsValues": settings, "zones": zones}
        self.created = []
        self._show_preview()
        self._highlight_zones()
        total = sum(len(z["points"]) for z in zones)
        message = f"Preview: {total} element(s) in {len(zones)} zone(s). Check them, then press Place."
        self.window.set_status(f"{message} {warning}".strip(), "warning" if warning else "ok")
        return True

    def _snapshot(self, settings: dict) -> tuple:
        return (tuple(sorted(settings.items())), self.part["name"] if self.part else None)

    def _show_preview(self) -> None:
        self.zone_tree.delete(*self.zone_tree.get_children())
        zones = self.preview["zones"] if self.preview else []
        for index, zone in enumerate(zones):
            self.zone_tree.insert("", "end", iid=str(index),
                                  values=(zone["label"], zone["story"], len(zone["points"]), zone["note"]))
        total = sum(len(z["points"]) for z in zones)
        empty = sum(1 for z in zones if not z["points"])
        text = f"{total} {self.part['name']} in {len(zones) - empty} zone(s)"
        if empty:
            text += f"; {empty} zone(s) get none"
        self.summary_label.configure(text=text + ". Select a zone to see its positions; double-click selects it.")
        if zones:
            self.zone_tree.selection_set("0")
        self._draw()

    def _draw(self) -> None:
        self.canvas.delete("all")
        if not self.preview or not self.preview["zones"]:
            return
        rows = self.zone_tree.selection()
        zones = [self.preview["zones"][int(r)] for r in rows] or self.preview["zones"][:1]
        xs = [p[0] for z in zones for p in z["outline"]["points"]]
        ys = [p[1] for z in zones for p in z["outline"]["points"]]
        width, height = max(10, self.canvas.winfo_width()), max(10, self.canvas.winfo_height())
        pad = 16
        scale = min((width - 2 * pad) / max(EPSILON, max(xs) - min(xs)),
                    (height - 2 * pad) / max(EPSILON, max(ys) - min(ys)))
        ox = pad + ((width - 2 * pad) - (max(xs) - min(xs)) * scale) / 2
        oy = pad + ((height - 2 * pad) - (max(ys) - min(ys)) * scale) / 2

        def to_canvas(p):
            return ox + (p[0] - min(xs)) * scale, height - (oy + (p[1] - min(ys)) * scale)

        for zone in zones:
            for contour, fill in [(zone["outline"], "#eef5ee")] + [(h, "white") for h in zone["holes"]]:
                if len(contour["points"]) > 2:
                    self.canvas.create_polygon(*[c for p in contour["points"] for c in to_canvas(p)],
                                               outline="#444444", fill=fill)
            for door in zone["doors"]:
                cx, cy = to_canvas(door["center"])
                r = max(2.0, door["width"] / 2 * scale)
                self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r, outline="#a15c00", dash=(3, 2))
            for x, y, angle in zone["points"]:
                cx, cy = to_canvas((x, y))
                self.canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4, fill="#1b7f3b", outline="")
                if self.preview["settingsValues"]["mode"] == "boundary":
                    # the side the element faces: its +y axis
                    self.canvas.create_line(cx, cy, cx - math.sin(angle) * 10, cy - math.cos(angle) * 10, fill="#1b7f3b")

    def _highlight_zones(self) -> None:
        zones = self.preview["zones"] if self.preview else []
        if not zones:
            return
        run_tapir("HighlightElements", {
            "elements": [{"elementId": z["elementId"]} for z in zones],
            "highlightedColors": [WITH_POINTS_COLOR if z["points"] else WITHOUT_POINTS_COLOR for z in zones],
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def _select_zone(self) -> None:
        row = self.zone_tree.focus()
        if row and self.preview:
            zone = self.preview["zones"][int(row)]
            select_elements([{"elementId": zone["elementId"]}])
            self.window.set_status(f"Selected {zone['label']} in Archicad.", "ok")

    # ---- placing ----

    def place(self) -> None:
        settings = self._settings()
        if settings is None:
            return
        if self.preview is None or self.preview["settings"] != self._snapshot(settings):
            if self.make_preview():
                self.window.set_status("The preview is updated to the current settings. Check it, then press Place again.",
                                       "warning")
            return
        if self.created:
            self.window.set_status("These elements are already placed. Press Preview to place again.", "warning")
            return
        zones = [z for z in self.preview["zones"] if z["points"]]
        total = sum(len(z["points"]) for z in zones)
        if not total:
            self.window.set_status("The preview has no positions to place anything at.", "warning")
            return
        layer = self.layers.get(self.layer.get())
        changes = ", ".join(f"{name} = {value}" for name, value in self.parameter_values.items())
        message = (f"Place {total} {self.part['name']} in {len(zones)} zone(s)?\n\n"
                   f"Layer: {self.layer.get()}\nParameters: {changes or 'the library part defaults'}")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
            return

        is_lamp = self.part["typeId"] == "Lamp"
        command, key = ("CreateLamps", "lampsData") if is_lamp else ("CreateObjects", "objectsData")
        height = settings["height"]
        items = [{
            "libraryPartName": self.part["name"],
            "coordinates": {"x": x, "y": y, "z": zone["z"] + height},
            "floorIndex": zone["floorIndex"],
            "angle": angle,
        } for zone in zones for x, y, angle in zone["points"]]

        created, errors = [], []
        with self.window.busy(f"Placing {total} element(s)..."):
            for chunk in batched(items, 500):
                self.window.set_status(f"Placing... {len(created)} / {total}")
                result = run_tapir(command, {key: chunk}).get("elements", [])
                created += [{"elementId": e["elementId"]} for e in result if isinstance(e, dict) and "elementId" in e]
                errors += item_errors(result)
            if created and layer is not None:
                self.window.set_status("Setting the layer...")
                for chunk in batched(created):
                    result = run_tapir("SetDetailsOfElements", {"elementsWithDetails": [
                        {"elementId": e["elementId"], "details": {"layerIndex": layer}} for e in chunk]})
                    errors += item_errors(result.get("executionResults", []))
            if created and self.parameter_values:
                self.window.set_status("Setting the parameters...")
                gdl = [{"name": name, "value": value} for name, value in self.parameter_values.items()]
                for chunk in batched(created, 200):
                    result = run_tapir("SetGDLParametersOfElements", {"elementsWithGDLParameters": [
                        {"elementId": e["elementId"], "gdlParameters": gdl} for e in chunk]})
                    errors += item_errors(result.get("executionResults", []))

        self.created = created
        if created:
            run_tapir("HighlightElements", {
                "elements": created,
                "highlightedColors": [CREATED_COLOR] * len(created),
                "nonHighlightedColor": DIMMED_COLOR,
            })
        for error in errors[:20]:
            print(f"Error: {error}")
        if errors:
            self.window.set_status(f"Placed {len(created)} of {total} element(s); {len(errors)} error(s), "
                                   f"the first: {errors[0]}", "error" if not created else "warning")
        else:
            self.window.set_status(f"Placed {len(created)} element(s). They are highlighted in blue.", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    def run(self) -> None:
        self.window.run(on_start=self.load, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    PlaceSymbolsInRooms().run()
