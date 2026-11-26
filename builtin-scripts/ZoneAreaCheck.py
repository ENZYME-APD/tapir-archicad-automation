# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "zones-areas"
# changes = "model"
# ///

"""
Zone Area Check

Compares the measured area of every zone with a target area read from a
property you choose, within a tolerance in percent. Lists the zones that are
over, under and on target, and highlights them in three colors. Select rows to
highlight only those zones, double-click one to select it in Archicad, and
export the list to Excel or CSV. Dimension perimeters places a linear dimension
along each straight edge of the zones, one for equal parallel edges.

Check only reads the project. Dimension perimeters is the only button that
changes it: it adds dimensions, and skips the edges that already carry one.
"""

import datetime
import math
import os
import re
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                run_tapir, select_elements)
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Zone Area Check"
DIMMED_COLOR = [200, 200, 200, 160]
SQUARE_FEET_IN_SQUARE_METERS = 0.09290304

OVER, UNDER, ON_TARGET, NO_TARGET, NO_AREA = "Over", "Under", "On target", "No target", "No area"
STATUS_ORDER = [OVER, UNDER, ON_TARGET, NO_TARGET, NO_AREA]
STATUS_COLORS = {
    OVER: (215, 25, 28),
    UNDER: (44, 123, 182),
    ON_TARGET: (26, 150, 65),
}
# The measured area: the label in the window and the key of GetQuantitiesOfElements' zone quantities.
AREAS = [
    ("Calculated area (as in the Zone Stamp)", "calculatedArea"),
    ("Net area", "netArea"),
    ("Gross area", "area"),
]
UNITS = [("m²", 1.0), ("ft²", SQUARE_FEET_IN_SQUARE_METERS)]

NO_FAVORITE, NO_LAYER = "(Dimension tool defaults)", "(Favorite or tool defaults)"
SAME_LENGTH = 0.001  # m: parallel edges this close in length get one dimension
SAME_POINT = 0.001  # m: witness points this close are the same point
PARALLEL = 1e-4  # the cross product of two unit directions below which they are parallel

_NUMBER = re.compile(r"[-+−]?\d[\d.,'\s  ]*")


def parse_number(text: str) -> float | None:
    """ The first number in a displayed value such as "1 234,5 m²" or "1,234.5". """
    match = _NUMBER.search(text)
    if not match:
        return None
    number = re.sub(r"[\s'  ]", "", match.group()).replace("−", "-").rstrip(".,")
    if "," in number and "." in number:
        # the separator that comes last is the decimal one
        thousands = "," if number.rfind(".") > number.rfind(",") else "."
        number = number.replace(thousands, "")
    elif number.count(",") > 1:
        number = number.replace(",", "")
    elif number.count(".") > 1:
        number = number.replace(".", "")
    try:
        return float(number.replace(",", "."))
    except ValueError:
        return None


def natural_key(text: str):
    """ Sorts "2" before "10". """
    return [(0, float(part), "") if part.isdigit() else (1, 0.0, part.lower()) for part in re.split(r"(\d+)", text)]


def to_hex(rgb) -> str:
    return "#{:02x}{:02x}{:02x}".format(*rgb[:3])


# ---- perimeter dimension geometry (plain tuples (x, y) in meters) ----

def _xy(coordinate: dict) -> tuple[float, float]:
    return float(coordinate["x"]), float(coordinate["y"])


def _inside_contour(point, contour) -> bool:
    """ Ray casting; the contour's points may repeat the first one at the end. """
    x, y = point
    inside = False
    for (x1, y1), (x2, y2) in zip(contour, contour[1:] + contour[:1]):
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            inside = not inside
    return inside


def inside_zone(point, contours) -> bool:
    """ Inside the outline (the first contour) and outside every hole. """
    return bool(contours) and _inside_contour(point, contours[0]) and not any(
        _inside_contour(point, hole) for hole in contours[1:])


def _parallel(u, v) -> bool:
    return abs(u[0] * v[1] - u[1] * v[0]) < PARALLEL


def _same_point(p, q) -> bool:
    return math.hypot(p[0] - q[0], p[1] - q[1]) <= SAME_POINT


def measures_edge(segment, start, end, unit) -> bool:
    """ Whether a placed dimension segment (floor ignored) measures the edge start-end along unit. """
    a, b, direction = segment
    return _parallel(direction, unit) and (
        (_same_point(a, start) and _same_point(b, end)) or (_same_point(a, end) and _same_point(b, start)))


def equal_parallel_edge(kept, start, unit, length) -> bool:
    """ Whether an edge kept in the zone is parallel to this one, as long, and on another line. """
    for other_start, other_unit, other_length in kept:
        if not _parallel(unit, other_unit) or abs(length - other_length) > SAME_LENGTH:
            continue
        # the distance of this edge's line from the other one's: 0 for edges on the same line
        if abs((start[0] - other_start[0]) * other_unit[1] - (start[1] - other_start[1]) * other_unit[0]) > SAME_POINT:
            return True
    return False


class ZoneAreaCheck:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Compares each zone's area with a target area property. Pick the property that holds the target, "
            "set the tolerance and press Check. Over, under and on target zones are highlighted in three colors. "
            "Select rows to highlight only those; double-click a row to select the zone. Dimension perimeters "
            "dimensions the edges of the selected rows' zones, or of every zone in the scope.",
            min_width=640, min_height=760)
        self.properties: list[dict] = []
        self.listed_properties: list[dict] = []
        self.zones: list[dict] = []  # {"elementId", "number", "name", "story", "measured", "target", "difference", "status"}
        self.shown: list[dict] = []
        self.property_name = ""
        self.dimension_notes: dict[str, str] = {}  # zone guid -> what Dimension perimeters did with it
        self.favorites: list[str] = []
        self.layers: dict[str, int] = {}
        self._build_body()
        self.window.add_button("Dimension perimeters", self.dimension_perimeters)
        self.window.add_button("Export...", self.export)
        self.window.add_button("Select in Archicad", self.select_listed)
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Check", self.check, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)
        body.rowconfigure(5, weight=2)

        zones = ttk.LabelFrame(body, text="Zones", padding=(10, 6))
        zones.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(zones, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))
        ttk.Label(zones, text="Measured area:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.area_kind = tk.StringVar(value=AREAS[0][0])
        ttk.Combobox(zones, textvariable=self.area_kind, values=[label for label, _ in AREAS], width=36,
                     state="readonly").grid(row=1, column=1, columnspan=3, sticky="w", padx=(6, 0), pady=(6, 0))

        properties = ttk.LabelFrame(body, text="Target area property", padding=(10, 6))
        properties.grid(row=1, column=0, sticky="nsew", pady=(8, 0))
        properties.columnconfigure(1, weight=1)
        properties.rowconfigure(1, weight=1)
        ttk.Label(properties, text="Search:").grid(row=0, column=0, sticky="w")
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_: self._list_properties())
        ttk.Entry(properties, textvariable=self.search).grid(row=0, column=1, sticky="ew", padx=(6, 0))
        self.property_tree = make_tree(properties, [("name", "Property", 230, "w"), ("group", "Group", 200, "w")], height=6)
        self.property_tree.configure(selectmode="browse")
        self.property_tree.container.grid(row=1, column=0, columnspan=2, sticky="nsew", pady=(6, 0))
        self.property_tree.bind("<Double-1>", lambda _e: self.window.guarded(self.check)())

        options = ttk.Frame(body, padding=(0, 8, 0, 0))
        options.grid(row=2, column=0, sticky="ew")
        ttk.Label(options, text="Target unit:").grid(row=0, column=0, sticky="w")
        self.unit = tk.StringVar(value=UNITS[0][0])
        unit = ttk.Combobox(options, textvariable=self.unit, values=[u for u, _ in UNITS], width=5, state="readonly")
        unit.grid(row=0, column=1, sticky="w", padx=(6, 0))
        unit.bind("<<ComboboxSelected>>", lambda _e: self._reclassify())
        ttk.Label(options, text="Tolerance (%):").grid(row=0, column=2, sticky="w", padx=(18, 0))
        self.tolerance = tk.StringVar(value="5")
        tolerance = ttk.Spinbox(options, textvariable=self.tolerance, from_=0, to=100, increment=1, width=5,
                                command=self._reclassify)
        tolerance.grid(row=0, column=3, sticky="w", padx=(6, 0))
        tolerance.bind("<Return>", lambda _e: (self._reclassify(), "break")[1])
        tolerance.bind("<FocusOut>", lambda _e: self._reclassify())
        self.problems_only = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Only over and under", variable=self.problems_only,
                        command=self._show_list).grid(row=0, column=4, sticky="w", padx=(18, 0))

        dimensions = ttk.LabelFrame(body, text="Perimeter dimensions", padding=(10, 6))
        dimensions.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        ttk.Label(dimensions, text="Favorite:").grid(row=0, column=0, sticky="w")
        self.favorite = tk.StringVar(value=NO_FAVORITE)
        self.favorite_box = ttk.Combobox(dimensions, textvariable=self.favorite, values=[NO_FAVORITE], width=26,
                                         state="readonly")
        self.favorite_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(dimensions, text="Layer:").grid(row=0, column=2, sticky="w", padx=(18, 0))
        self.layer = tk.StringVar(value=NO_LAYER)
        self.layer_box = ttk.Combobox(dimensions, textvariable=self.layer, values=[NO_LAYER], width=26, state="readonly")
        self.layer_box.grid(row=0, column=3, sticky="w", padx=(6, 0))
        ttk.Label(dimensions, text="Offset from the edge (m):").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.offset = tk.StringVar(value="0.3")
        ttk.Spinbox(dimensions, textvariable=self.offset, from_=0.05, to=5, increment=0.05, width=6).grid(
            row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Label(dimensions, text="Shortest edge (m):").grid(row=1, column=2, sticky="w", padx=(18, 0), pady=(6, 0))
        self.min_length = tk.StringVar(value="0.1")
        ttk.Spinbox(dimensions, textvariable=self.min_length, from_=0, to=10, increment=0.05, width=6).grid(
            row=1, column=3, sticky="w", padx=(6, 0), pady=(6, 0))
        self.include_holes = tk.BooleanVar(value=False)
        ttk.Checkbutton(dimensions, text="Also the edges of holes", variable=self.include_holes).grid(
            row=2, column=0, columnspan=4, sticky="w", pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 8, 0, 4))
        summary.grid(row=4, column=0, sticky="ew")
        self.count_labels: dict[str, ttk.Label] = {}
        for column, status in enumerate([OVER, UNDER, ON_TARGET, NO_TARGET]):
            cell = ttk.Frame(summary)
            cell.grid(row=0, column=column, sticky="w", padx=(0, 22))
            label = ttk.Label(cell, text="–", style="Big.TLabel")
            if status in STATUS_COLORS:
                label.configure(foreground=to_hex(STATUS_COLORS[status]))
            label.grid(row=0, column=0, sticky="w")
            ttk.Label(cell, text=status.lower(), style="Muted.TLabel").grid(row=1, column=0, sticky="w")
            self.count_labels[status] = label

        self.tree = make_tree(body, [
            ("number", "Number", 70, "w"), ("name", "Name", 170, "w"), ("story", "Story", 110, "w"),
            ("measured", "Measured m²", 95, "e"), ("target", "Target m²", 85, "e"),
            ("difference", "Difference", 80, "e"), ("status", "Status", 80, "w"),
            ("dimensions", "Dimensions", 150, "w"),
        ], height=10)
        self.tree.container.grid(row=5, column=0, sticky="nsew")
        for status, color in STATUS_COLORS.items():
            self.tree.tag_configure(status, foreground=to_hex(color))
        self.tree.tag_configure(NO_TARGET, foreground="#777777")
        self.tree.tag_configure(NO_AREA, foreground="#777777")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._apply_highlight)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- properties ----

    def load_properties(self) -> None:
        with self.window.busy("Reading the properties..."):
            properties = run_tapir("GetAllProperties").get("properties", [])
            self._load_dimension_options()
        self.properties = sorted(
            (p for p in properties if "propertyId" in p),
            key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                           p.get("propertyName", "").lower()))
        self._list_properties()
        self.window.set_status(f"{len(self.properties)} properties. Pick the one with the target area and press Check.")

    def _load_dimension_options(self) -> None:
        """ The Dimension favorites and the layers for Dimension perimeters; a failure only empties a list. """
        try:
            self.favorites = sorted(run_tapir("GetFavoritesByType", {"elementType": "Dimension"}).get("favorites", []),
                                    key=str.lower)
        except ArchicadError as e:
            print(f"No Dimension favorites: {e}")
        self.favorite_box.configure(values=[NO_FAVORITE] + self.favorites)
        try:
            layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
        except ArchicadError as e:
            print(f"No layers: {e}")
            layers = []
        self.layers = {}
        for layer in sorted(layers, key=lambda a: str(a.get("name", "")).lower()):
            label = str(layer.get("name", ""))
            if label in self.layers:
                label = f"{label} ({layer['index']})"
            self.layers[label] = int(layer["index"])
        self.layer_box.configure(values=[NO_LAYER] + list(self.layers))

    def _list_properties(self) -> None:
        words = self.search.get().lower().split()
        self.listed_properties = [
            p for p in self.properties
            if all(w in f"{p.get('propertyGroupName', '')} {p.get('propertyName', '')}".lower() for w in words)]
        self.property_tree.delete(*self.property_tree.get_children())
        for index, p in enumerate(self.listed_properties):
            self.property_tree.insert("", "end", iid=str(index), values=(p.get("propertyName", ""), p.get("propertyGroupName", "")))
        if len(self.listed_properties) == 1:
            self.property_tree.selection_set("0")

    def _chosen_property(self) -> dict | None:
        rows = self.property_tree.selection()
        return self.listed_properties[int(rows[0])] if rows else None

    # ---- checking ----

    def check(self) -> None:
        prop = self._chosen_property()
        if prop is None:
            self.window.set_status("Pick the target area property in the list first.", "warning")
            return
        area_key = dict(AREAS)[self.area_kind.get()]
        with self.window.busy("Reading the zones..."):
            zones = get_elements(self.scope.get(), ["Zone"])
            if not zones:
                self.zones = []
                self._reclassify()
                run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
                self.window.set_status("There is no zone to check.", "warning")
                return
            stories = get_story_names()
            details = get_details_of_elements(zones, ["floorIndex", "details"])
            measured: list[float | None] = []
            targets: list[float | None] = []
            for chunk in batched(zones):
                self.window.set_status(f"Reading the areas... {len(measured)} / {len(zones)}")
                quantities = run_tapir("GetQuantitiesOfElements", {"elements": chunk}).get("quantitiesOfElements", [])
                for item in quantities:
                    value = (item.get("quantities") or {}).get(area_key) if "error" not in item else None
                    measured.append(value if isinstance(value, (int, float)) else None)
                values = run_tapir("GetPropertyValuesOfElements", {
                    "elements": chunk, "properties": [{"propertyId": prop["propertyId"]}]})
                for entry in values.get("propertyValuesForElements", []):
                    item = (entry.get("propertyValues") or [{}])[0]
                    text = (item.get("propertyValue") or {}).get("value", "")
                    targets.append(parse_number(str(text)) if text not in (None, "") else None)
            if len(measured) != len(zones) or len(targets) != len(zones):
                raise ArchicadError("Archicad returned a different number of results than zones asked for.")

        self.zones = []
        for zone, detail, area, target in zip(zones, details, measured, targets):
            info = detail.get("details") or {}
            self.zones.append({
                "elementId": zone["elementId"],
                "number": info.get("numberStr", ""),
                "name": info.get("name", ""),
                "floorIndex": detail.get("floorIndex", 0),
                "story": stories.get(detail.get("floorIndex"), str(detail.get("floorIndex", ""))),
                "measured": area,
                "targetValue": target,
                "target": None,
            })
        self.zones.sort(key=lambda z: (z["floorIndex"], natural_key(z["number"]), natural_key(z["name"])))
        self.property_name = prop.get("propertyName", "")
        self.tree.selection_set(())
        self._reclassify()

    def _tolerance(self) -> float:
        try:
            return max(0.0, float(self.tolerance.get().replace(",", ".")))
        except ValueError:
            return 5.0

    def _reclassify(self) -> None:
        tolerance = self._tolerance()
        unit_factor = dict(UNITS)[self.unit.get()]
        for zone in self.zones:
            # the property's number is in the chosen unit; the list shows square meters
            zone["target"] = zone["targetValue"] * unit_factor if zone["targetValue"] is not None else None
            measured, target = zone["measured"], zone["target"]
            zone["difference"] = None
            if measured is None:
                zone["status"] = NO_AREA
            elif target is None or target <= 0:
                zone["status"] = NO_TARGET
            else:
                difference = (measured - target) / target * 100
                zone["difference"] = difference
                # a little slack so that a zone exactly at the tolerance counts as on target
                if difference > tolerance + 1e-9:
                    zone["status"] = OVER
                elif difference < -tolerance - 1e-9:
                    zone["status"] = UNDER
                else:
                    zone["status"] = ON_TARGET
        self._show_list()

    def _show_list(self) -> None:
        self.tree.delete(*self.tree.get_children())
        problems_only = self.problems_only.get()
        self.shown = [z for z in self.zones if not problems_only or z["status"] in (OVER, UNDER)]
        for index, zone in enumerate(self.shown):
            self.tree.insert("", "end", iid=str(index), tags=(zone["status"],), values=(
                zone["number"], zone["name"], zone["story"],
                f"{zone['measured']:.2f}" if zone["measured"] is not None else "–",
                f"{zone['target']:.2f}" if zone["target"] is not None else "–",
                f"{zone['difference']:+.1f} %" if zone["difference"] is not None else "–",
                zone["status"], self.dimension_notes.get(zone["elementId"]["guid"], "")))
        counts = {status: sum(1 for z in self.zones if z["status"] == status) for status in STATUS_ORDER}
        for status, label in self.count_labels.items():
            label.configure(text=str(counts[status] + (counts[NO_AREA] if status == NO_TARGET else 0)) if self.zones else "–")
        self.window.guarded(self._apply_highlight)()

    def _apply_highlight(self) -> None:
        rows = self.tree.selection()
        zones = [self.shown[int(r)] for r in rows] if rows else self.shown
        zones = [z for z in zones if z["status"] in STATUS_COLORS]
        if not zones:
            if self.zones:
                run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
                self.window.set_status("None of these zones has both an area and a target to compare.", "warning")
            return
        run_tapir("HighlightElements", {
            "elements": [{"elementId": z["elementId"]} for z in zones],
            "highlightedColors": [list(STATUS_COLORS[z["status"]]) + [255] for z in zones],
            "nonHighlightedColor": DIMMED_COLOR,
        })
        counts = {status: sum(1 for z in self.zones if z["status"] == status) for status in STATUS_ORDER}
        if rows:
            self.window.set_status(f"Highlighted {len(zones)} selected zone(s).", "ok")
        else:
            missing = counts[NO_TARGET] + counts[NO_AREA]
            note = f" {missing} zone(s) without a target or area are not highlighted." if missing else ""
            self.window.set_status(
                f"{counts[OVER]} over, {counts[UNDER]} under, {counts[ON_TARGET]} on target "
                f"(±{self._tolerance():g} % of {self.property_name}).{note}", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    # ---- perimeter dimensions ----

    @staticmethod
    def _number(variable: tk.StringVar, default: float) -> float:
        try:
            return max(0.0, float(variable.get().replace(",", ".")))
        except ValueError:
            return default

    @staticmethod
    def _polygon_vertex_ids(zones: list[dict]) -> list[dict | None]:
        """ GetPolygonVertexIdsOfElements per zone, None where it has none; all None when the command fails. """
        results: list[dict | None] = []
        try:
            for chunk in batched(zones):
                items = run_tapir("GetPolygonVertexIdsOfElements", {"elements": chunk}).get("polygonVertexIdsOfElements", [])
                results += [item if isinstance(item, dict) and "error" not in item else None for item in items]
        except ArchicadError as e:
            print(f"The dimensions are static, not associated with the zones: {e}")
            return [None] * len(zones)
        return results if len(results) == len(zones) else [None] * len(zones)

    @staticmethod
    def _zone_contours(info: dict, vertex_ids: dict | None) -> list[dict]:
        """ The outline and the holes, each {"points": [{"xy", "inIndex", "vertexId"}], "arcs": {begIndex}}.
        The points come with their vertex IDs when GetPolygonVertexIdsOfElements gave them for the same
        polygon; otherwise only with their coordinates, for static witness points. """
        outlines = [info.get("polygonOutline") or []] + [h.get("polygonOutline") or [] for h in info.get("holes") or []]
        arcs = [info.get("polygonArcs") or []] + [h.get("polygonArcs") or [] for h in info.get("holes") or []]
        contours = (vertex_ids or {}).get("contours") or []
        with_ids = len(contours) == len(outlines) and all(
            len(c.get("points") or []) == len(o) for c, o in zip(contours, outlines))
        result = []
        for index, outline in enumerate(outlines):
            if with_ids:
                points = [{"xy": _xy(p["coordinate"]), "inIndex": p["inIndex"], "vertexId": p["vertexId"]}
                          for p in contours[index]["points"]]
            else:
                points = [{"xy": _xy(c)} for c in outline]
            result.append({"points": points, "arcs": {int(a["begIndex"]) for a in arcs[index] if "begIndex" in a}})
        return result

    @staticmethod
    def _existing_segments(floors: set) -> list[tuple]:
        """ (floorIndex, (point, point, unit direction)) for every pair of neighbouring witness points of the
        linear dimensions already on the given stories, so running twice adds no second set. """
        dimensions = run_tapir("GetElementsByType", {"elementType": "Dimension"}).get("elements", [])
        if not dimensions:
            return []
        dimension_floors = [d.get("floorIndex") for d in get_details_of_elements(dimensions, ["floorIndex"])]
        data = []
        for chunk in batched(dimensions):
            data += run_tapir("GetDimensionData", {"elements": chunk}).get("dimensionsData", [])
        segments = []
        for floor, dimension in zip(dimension_floors, data):
            if floor not in floors or not isinstance(dimension, dict) or "error" in dimension:
                continue
            dx, dy = _xy(dimension.get("direction") or {"x": 0, "y": 0})
            size = math.hypot(dx, dy)
            if size == 0:
                continue
            unit = (dx / size, dy / size)
            points = sorted((_xy(w["coordinate"]) for w in dimension.get("witnessPoints") or [] if "coordinate" in w),
                            key=lambda p: p[0] * unit[0] + p[1] * unit[1])
            segments += [(floor, (a, b, unit)) for a, b in zip(points, points[1:])]
        return segments

    def _create_dimensions(self, planned: list[tuple], notes: dict[str, dict]) -> tuple[list[str], int]:
        """ Creates the planned (zone guid, associative dimension, static dimension) items and counts the placed
        ones per zone in notes. Returns the error messages and how many had to be static. """
        errors = []
        static_count = 0
        for index, chunk in enumerate(batched(planned, 200)):
            self.window.set_status(f"Placing the dimensions... {index * 200} / {len(planned)}")
            results = run_tapir("CreateAssociativeDimensions", {"dimensionsData": [d for _, d, _ in chunk]}).get("elements", [])
            # an associative dimension Archicad refuses is placed again with static witness points
            retry = [(item, result) for item, result in zip(chunk, results)
                     if "error" in result and item[1] is not item[2]]
            if retry:
                again = run_tapir("CreateAssociativeDimensions", {"dimensionsData": [s for (_, _, s), _ in retry]}).get("elements", [])
                fixed = {id(item): result for (item, _), result in zip(retry, again)}
                static_count += sum(1 for result in again if "error" not in result)
                results = [fixed.get(id(item), result) for item, result in zip(chunk, results)]
            for (guid, _, _), result in zip(chunk, results):
                if "error" in result:
                    errors.append(str((result.get("error") or {}).get("message") or "Unknown error"))
                else:
                    notes[guid]["placed"] += 1
        return errors, static_count

    def dimension_perimeters(self) -> None:
        rows = self.tree.selection()
        if rows:
            zones = [{"elementId": self.shown[int(r)]["elementId"]} for r in rows]
        else:
            with self.window.busy("Reading the zones..."):
                zones = get_elements(self.scope.get(), ["Zone"])
        if not zones:
            self.window.set_status("There is no zone to dimension.", "warning")
            return
        offset = self._number(self.offset, 0.3) or 0.3
        min_length = max(self._number(self.min_length, 0.1), SAME_POINT)
        include_holes = self.include_holes.get()
        settings = {}
        if self.favorite.get() in self.favorites:
            settings["favoriteName"] = self.favorite.get()
        if self.layer.get() in self.layers:
            settings["layerIndex"] = self.layers[self.layer.get()]

        with self.window.busy("Reading the zone outlines and the dimensions already placed..."):
            details = get_details_of_elements(zones, ["floorIndex", "details"])
            vertex_ids = self._polygon_vertex_ids(zones)
            placed = self._existing_segments({d.get("floorIndex", 0) for d in details})

        planned = []  # (zone guid, associative dimension, static dimension)
        counts = {"equal": 0, "existing": 0, "curved": 0, "short": 0}
        notes: dict[str, dict] = {}
        reference_line = []
        for zone, detail, ids in zip(zones, details, vertex_ids):
            guid = zone["elementId"]["guid"]
            info = detail.get("details") or {}
            if info.get("boundaryMethod") == "ReferenceLine":
                self.dimension_notes[guid] = "Not dimensioned: reference line"
                reference_line.append(f"{info.get('numberStr', '')} {info.get('name', '')}".strip() or guid)
                continue
            floor = detail.get("floorIndex", 0)
            contours = self._zone_contours(info, ids)
            geometry = [[p["xy"] for p in c["points"]] for c in contours]
            note = notes[guid] = {"placed": 0, "equal": 0, "existing": 0, "curved": 0}
            kept = []  # (start, unit, length) of the edges of this zone that have a dimension
            for contour in contours[:None if include_holes else 1]:
                points = contour["points"]
                if len(points) > 1 and _same_point(points[0]["xy"], points[-1]["xy"]):
                    points = points[:-1]  # the closing point repeats the first one
                if len(points) < 3:
                    continue
                for i, first in enumerate(points):
                    if i in contour["arcs"]:
                        counts["curved"] += 1
                        note["curved"] += 1
                        continue
                    second = points[(i + 1) % len(points)]
                    start, end = first["xy"], second["xy"]
                    length = math.hypot(end[0] - start[0], end[1] - start[1])
                    if length < min_length:
                        counts["short"] += 1
                        continue
                    unit = ((end[0] - start[0]) / length, (end[1] - start[1]) / length)
                    if equal_parallel_edge(kept, start, unit, length):
                        counts["equal"] += 1
                        note["equal"] += 1
                        continue
                    kept.append((start, unit, length))
                    if any(f == floor and measures_edge(segment, start, end, unit) for f, segment in placed):
                        counts["existing"] += 1
                        note["existing"] += 1
                        continue
                    placed.append((floor, (start, end, unit)))
                    # the side of the edge that is inside the zone, so the text sits in the room
                    middle = ((start[0] + end[0]) / 2, (start[1] + end[1]) / 2)
                    normal = (-unit[1], unit[0])
                    probe = min(0.005, length / 4)
                    if not inside_zone((middle[0] + normal[0] * probe, middle[1] + normal[1] * probe), geometry):
                        normal = (-normal[0], -normal[1])
                    dimension = {
                        **settings,
                        "referencePoint": {"x": middle[0] + normal[0] * offset, "y": middle[1] + normal[1] * offset},
                        "direction": {"x": unit[0], "y": unit[1]},
                        "floorIndex": floor,
                    }
                    static = {**dimension, "witnessPoints": [{"coordinate": {"x": p[0], "y": p[1]}} for p in (start, end)]}
                    if "vertexId" in first and "vertexId" in second:
                        dimension["witnessPoints"] = [
                            {"elementId": zone["elementId"], "inIndex": p["inIndex"], "nodeId": p["vertexId"]}
                            for p in (first, second)]
                    else:
                        dimension = static
                    planned.append((guid, dimension, static))

        with self.window.busy("Placing the dimensions..."):
            errors, static_count = self._create_dimensions(planned, notes)

        total = 0
        for guid, note in notes.items():
            total += note["placed"]
            parts = [f"{note['placed']} placed"]
            parts += [f"{note['equal']} equal"] if note["equal"] else []
            parts += [f"{note['existing']} already there"] if note["existing"] else []
            parts += [f"{note['curved']} curved"] if note["curved"] else []
            self.dimension_notes[guid] = ", ".join(parts)
        # only the Dimensions column, so the selected rows and the highlight stay as they are
        for index, zone in enumerate(self.shown):
            self.tree.set(str(index), "dimensions", self.dimension_notes.get(zone["elementId"]["guid"], ""))

        message = (f"Placed {total} dimension(s) on {len(notes)} zone(s). Skipped {counts['equal']} as equal to a "
                   f"parallel edge, {counts['existing']} already dimensioned, {counts['curved']} curved and "
                   f"{counts['short']} short edge(s).")
        if static_count:
            message += f" {static_count} are static: Archicad did not associate them with the zone."
        if reference_line:
            message += (f" {len(reference_line)} zone(s) measured to the walls' reference lines are not dimensioned: "
                        + ", ".join(reference_line[:5]) + ("..." if len(reference_line) > 5 else "") + ".")
        if errors:
            message += f" {len(errors)} failed: {errors[0]}"
        self.window.set_status(message, "warning" if errors or reference_line else "ok")

    # ---- selection and export ----

    def select_listed(self) -> None:
        rows = self.tree.selection()
        zones = [self.shown[int(r)] for r in rows] if rows else self.shown
        if not zones:
            self.window.set_status("There is nothing to select. Press Check first.", "warning")
            return
        select_elements([{"elementId": z["elementId"]} for z in zones])
        self.window.set_status(f"Selected {len(zones)} zone(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row:
            zone = self.shown[int(row)]
            select_elements([{"elementId": zone["elementId"]}])
            self.window.set_status(f"Selected zone {zone['number']} {zone['name']}.".replace("  ", " "), "ok")

    def export(self) -> None:
        if not self.zones:
            self.window.set_status("Press Check first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the zone areas",
                                            initialfile=f"Zone area check {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        rows = [[z["number"], z["name"], z["story"],
                 round(z["measured"], 2) if z["measured"] is not None else "",
                 round(z["target"], 2) if z["target"] is not None else "",
                 round(z["difference"], 1) if z["difference"] is not None else "",
                 z["status"], self.dimension_notes.get(z["elementId"]["guid"], ""), z["elementId"]["guid"]]
                for z in self.zones]
        write_table(path, ["Number", "Name", "Story", "Measured m²", "Target m²", "Difference %", "Status", "Dimensions",
                           "GUID"],
                    rows, "Zone area check")
        self.window.set_status(f"Exported {len(rows)} zone(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_properties, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ZoneAreaCheck().run()
