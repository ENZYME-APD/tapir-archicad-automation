# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "zones-areas"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "files"
# ///

"""
Zone Area Dashboard

Shows the areas of the project's zones (all of them, or the selected ones) grouped by any zone
metric: zone name, number, category, story, a custom property or a classification. Summary cards
give the total, the number of zones and the area and share of each group; the table lists the
groups and, expanded, their zones; charts show the share per group (donut), the area per group
(bars), the area per story split by the colour-by metric and a treemap of the zones. Clicking a
card, a row or a chart item highlights those zones in Archicad in the colour of their colour-by
value and fades the rest. Exports to Excel and CSV. It changes
nothing in the project apart from the highlight and, on request, the selection.
"""

import colorsys
import datetime
import os
import re
import sys
import tkinter as tk
from tkinter import filedialog, font as tkfont, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                run_command, run_tapir, select_elements)
from utilities.export import FILE_TYPES, write_table, write_workbook
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Zone Area Dashboard"
NO_VALUE = "(no value)"
DIMMED_COLOR = [200, 200, 200, 160]
NO_VALUE_COLOR = (170, 170, 170)
SQUARE_FEET_IN_SQUARE_METERS = 0.09290304
MAX_CARDS = 12
NO_FILTER = "(no filter)"
# the charts are drawn on white, like a printed page, in the light and the dark theme
CHART_BACKGROUND = "#ffffff"
CHART_TEXT = "#1f1f1f"
CHART_MUTED = "#5f6368"
CHART_SELECTED = "#1f1f1f"

# The area: the label in the window and the key of GetQuantitiesOfElements' zone quantities (all in m²).
AREAS = [
    ("Measured area", "area"),
    ("Net area", "netArea"),
    ("Calculated area (as in the Zone Stamp)", "calculatedArea"),
]
UNITS = [("m²", 1.0), ("ft²", SQUARE_FEET_IN_SQUARE_METERS)]

# The metrics every zone has; properties and classification systems are read from the project.
BUILT_IN_METRICS = [
    {"key": "name", "label": "Zone name", "kind": "builtIn"},
    {"key": "number", "label": "Zone number", "kind": "builtIn"},
    {"key": "category", "label": "Zone category", "kind": "builtIn"},
    {"key": "story", "label": "Story", "kind": "builtIn"},
]

# Colour-blind friendly first (Okabe-Ito and Tableau), then generated hues.
PALETTE = [
    (0, 114, 178), (230, 159, 0), (0, 158, 115), (204, 121, 167), (86, 180, 233), (213, 94, 0),
    (240, 228, 66), (78, 121, 167), (242, 142, 43), (225, 87, 89), (118, 183, 178), (89, 161, 79),
    (237, 201, 72), (176, 122, 161), (255, 157, 167), (156, 117, 95),
]


# ---- the data, without Archicad or a window (tested by archicad-addon/Test/test_zone_area_dashboard.py) ----

def natural_key(text: str):
    """ Sorts "2" before "10". """
    return [(0, float(part), "") if part.isdigit() else (1, 0.0, part.lower()) for part in re.split(r"(\d+)", text)]


def value_sort_key(value: str, order: dict[str, int] | None = None):
    """ The values in their natural order, or in the given order (stories by floor), "(no value)" last. """
    if value == NO_VALUE:
        return (2, 0, [])
    if order and value in order:
        return (0, order[value], [])
    return (1, 0, natural_key(value))


def clean_value(value) -> str:
    """ A value as text; an empty one is "(no value)", so a zone without it still counts. """
    if value is None:
        return NO_VALUE
    if isinstance(value, list):
        value = ", ".join(str(v) for v in value)
    text = str(value).strip()
    return text if text else NO_VALUE


def metric_value(zone: dict, key: str) -> str:
    return clean_value(zone["values"].get(key))


def zone_area(zone: dict, area_key: str) -> float | None:
    value = zone["areas"].get(area_key)
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def filter_zones(zones: list[dict], filters: dict[str, set[str]]) -> list[dict]:
    """ The zones whose value of each filter metric is one of the allowed values; an empty set allows all. """
    active = {key: allowed for key, allowed in filters.items() if allowed}
    return [z for z in zones if all(metric_value(z, key) in allowed for key, allowed in active.items())]


def totals(zones: list[dict], area_key: str) -> dict:
    """ The total area (m²) of the zones, their number, and how many have no area. """
    areas = [zone_area(z, area_key) for z in zones]
    return {"area": sum(a for a in areas if a is not None), "count": len(zones),
            "missing": sum(1 for a in areas if a is None)}


def group_zones(zones: list[dict], key: str, area_key: str, order: dict[str, int] | None = None) -> list[dict]:
    """ One group per value of the metric, largest area first: value, zones, area (m²), count,
    missing (zones without area) and share (of the total area of all the zones). """
    by_value: dict[str, list[dict]] = {}
    for zone in zones:
        by_value.setdefault(metric_value(zone, key), []).append(zone)
    total = totals(zones, area_key)["area"]
    groups = []
    for value, members in by_value.items():
        group = totals(members, area_key)
        group.update({"value": value, "zones": members, "share": group["area"] / total if total > 0 else 0.0})
        groups.append(group)
    groups.sort(key=lambda g: (-g["area"], value_sort_key(g["value"], order)))
    return groups


def largest_and_smallest(groups: list[dict]) -> tuple[dict | None, dict | None]:
    if not groups:
        return None, None
    return max(groups, key=lambda g: g["area"]), min(groups, key=lambda g: g["area"])


def palette_color(index: int) -> tuple[int, int, int]:
    if index < len(PALETTE):
        return PALETTE[index]
    # golden-ratio hues, so neighbours differ
    hue = (index - len(PALETTE)) * 0.618033988749895 % 1.0
    red, green, blue = colorsys.hsv_to_rgb(hue, 0.55, 0.85)
    return round(red * 255), round(green * 255), round(blue * 255)


def assign_colors(values, order: dict[str, int] | None = None) -> dict[str, tuple[int, int, int]]:
    """ One colour per value, the same whatever is filtered or shown: the values are numbered in
    their sort order. "(no value)" is grey. """
    colors = {}
    for index, value in enumerate(sorted({v for v in values if v != NO_VALUE}, key=lambda v: value_sort_key(v, order))):
        colors[value] = palette_color(index)
    colors[NO_VALUE] = NO_VALUE_COLOR
    return colors


def to_hex(rgb) -> str:
    return "#{:02x}{:02x}{:02x}".format(*rgb[:3])


def in_unit(area_m2: float | None, factor: float) -> float | None:
    return None if area_m2 is None else area_m2 / factor


def group_table(groups: list[dict], metric_label: str, area_header: str, factor: float) -> tuple[list[str], list[list]]:
    """ The groups as a sheet: one row per group and a total row. """
    headers = [metric_label, area_header, "Zones", "Share %", "Zones without area"]
    rows = [[g["value"], round(g["area"] / factor, 2), g["count"], round(g["share"] * 100, 1), g["missing"]]
            for g in groups]
    total_area = sum(g["area"] for g in groups)
    rows.append(["Total", round(total_area / factor, 2), sum(g["count"] for g in groups),
                 100.0 if total_area > 0 else 0.0, sum(g["missing"] for g in groups)])
    return headers, rows


def zone_table(zones: list[dict], metrics: list[dict], area_key: str, area_header: str,
               factor: float) -> tuple[list[str], list[list]]:
    """ One row per zone: every metric, the chosen area and the GUID. """
    headers = [m["label"] for m in metrics] + [area_header, "GUID"]
    rows = []
    for zone in zones:
        area = in_unit(zone_area(zone, area_key), factor)
        rows.append([metric_value(zone, m["key"]) for m in metrics]
                    + [round(area, 2) if area is not None else "", zone["guid"]])
    return headers, rows


def format_area(area_m2: float | None, factor: float, unit: str) -> str:
    return "–" if area_m2 is None else f"{area_m2 / factor:,.2f} {unit}"


def is_additive_click(event) -> bool:
    """ Ctrl-click, or Cmd-click on macOS, adds to the highlight instead of replacing it. """
    state = getattr(event, "state", 0)
    if not isinstance(state, int):
        return False
    return bool(state & 0x0004) or (sys.platform == "darwin" and bool(state & 0x0008))


# ---- the charts' data, without a window (tested as well) ----

def donut_slices(groups: list[dict]) -> list[dict]:
    """ One slice per group with an area, clockwise from 12 o'clock in the order of the groups:
    value, start and extent in Tk's degrees (counter-clockwise from 3 o'clock) and share. """
    total = sum(g["area"] for g in groups if g["area"] > 0)
    slices, done = [], 0.0
    if total <= 0:
        return slices
    for group in groups:
        if group["area"] <= 0:
            continue
        extent = group["area"] / total * 360.0
        slices.append({"value": group["value"], "start": 90.0 - done - extent, "extent": extent,
                       "share": group["area"] / total})
        done += extent
    return slices


def story_stack(zones: list[dict], area_key: str, colour_key: str, story_order: dict[str, int] | None = None,
                colour_order: dict[str, int] | None = None) -> list[dict]:
    """ One entry per story, lowest first: story, area (m²), count and parts, one part per colour-by
    value in the story (value, area, zones), in the values' sort order, so every story stacks alike. """
    stories: dict[str, dict] = {}
    for zone in zones:
        story = metric_value(zone, "story")
        value = metric_value(zone, colour_key)
        entry = stories.setdefault(story, {"story": story, "area": 0.0, "count": 0, "parts": {}})
        part = entry["parts"].setdefault(value, {"value": value, "area": 0.0, "zones": []})
        part["zones"].append(zone)
        entry["count"] += 1
        area = zone_area(zone, area_key)
        if area is not None:
            part["area"] += area
            entry["area"] += area
    result = []
    for story in sorted(stories, key=lambda s: value_sort_key(s, story_order)):
        entry = stories[story]
        entry["parts"] = [entry["parts"][v] for v in sorted(entry["parts"], key=lambda v: value_sort_key(v, colour_order))]
        result.append(entry)
    return result


def story_table(stack: list[dict], colour_label: str, area_header: str, factor: float,
                colour_order: dict[str, int] | None = None) -> tuple[list[str], list[list]]:
    """ The stack as a sheet: one row per story, one column per colour-by value, a total column and row. """
    values = sorted({p["value"] for entry in stack for p in entry["parts"]}, key=lambda v: value_sort_key(v, colour_order))
    headers = [f"Story / {colour_label}, {area_header}"] + values + ["Total"]
    rows = []
    for entry in stack:
        areas = {p["value"]: p["area"] for p in entry["parts"]}
        rows.append([entry["story"]] + [round(areas.get(v, 0.0) / factor, 2) for v in values]
                    + [round(entry["area"] / factor, 2)])
    column_totals = [sum(p["area"] for entry in stack for p in entry["parts"] if p["value"] == v) for v in values]
    rows.append(["Total"] + [round(a / factor, 2) for a in column_totals]
                + [round(sum(e["area"] for e in stack) / factor, 2)])
    return headers, rows


def squarify(sizes: list[float], x: float, y: float, width: float, height: float) -> list[tuple[float, float, float, float]]:
    """ A squarified treemap: one rectangle (x, y, width, height) per size, in the order of the sizes,
    their areas proportional to the sizes and together filling the box. A size of 0 or less gets an
    empty rectangle. """
    rects: list[tuple[float, float, float, float]] = [(x, y, 0.0, 0.0)] * len(sizes)
    items = sorted((i for i, s in enumerate(sizes) if s > 0), key=lambda i: -sizes[i])
    total = sum(sizes[i] for i in items)
    if not items or width <= 0 or height <= 0:
        return rects
    remaining = [(i, sizes[i] * width * height / total) for i in items]

    def worst(row: list[tuple[int, float]], side: float) -> float:
        row_sum = sum(a for _i, a in row)
        return max(max(side * side * a / (row_sum * row_sum), row_sum * row_sum / (side * side * a)) for _i, a in row)

    while remaining:
        side = min(width, height)
        if side <= 0:
            break
        count = 1
        while count < len(remaining) and worst(remaining[:count + 1], side) <= worst(remaining[:count], side):
            count += 1
        row, remaining = remaining[:count], remaining[count:]
        row_sum = sum(a for _i, a in row)
        if width >= height:
            # a column along the left side
            column_width = row_sum / height
            top = y
            for index, area in row:
                rects[index] = (x, top, column_width, area / column_width)
                top += area / column_width
            x, width = x + column_width, width - column_width
        else:
            # a row along the top
            row_height = row_sum / width
            left = x
            for index, area in row:
                rects[index] = (left, y, area / row_height, row_height)
                left += area / row_height
            y, height = y + row_height, height - row_height
    return rects


def text_color(rgb) -> str:
    """ Black or white, whichever reads better on the colour. """
    red, green, blue = (c / 255 for c in rgb[:3])
    return "#000000" if 0.299 * red + 0.587 * green + 0.114 * blue > 0.6 else "#ffffff"


def faded(rgb, amount: float = 0.7) -> tuple[int, int, int]:
    """ The colour mixed with white, as the charts show what is not highlighted. """
    return tuple(round(c + (255 - c) * amount) for c in rgb[:3])


# ---- the window ----

class ZoneAreaDashboard:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "The area of the zones, grouped by any zone metric. Click a card or a table row to highlight "
            "those zones in Archicad in their colour and fade the rest; Ctrl-click (Cmd-click on macOS) adds to "
            "the highlight, clicking again removes it. The tabs next to the table show charts; click a slice, bar, "
            "tile or legend entry to highlight its zones the same way.",
            min_width=900, min_height=760)
        self.zones: list[dict] = []  # {"elementId", "guid", "floorIndex", "areas": {key: m²}, "values": {metric: text}}
        self.metrics: list[dict] = list(BUILT_IN_METRICS)
        self.loaded: set[str] = set()
        self.orders: dict[str, dict[str, int]] = {}
        self._color_cache: dict[str, dict[str, tuple[int, int, int]]] = {}
        self.shown: list[dict] = []
        self.groups: list[dict] = []
        self.row_zones: dict[str, list[dict]] = {}
        self.group_rows: dict[str, str] = {}
        self.swatches: dict[tuple, tk.PhotoImage] = {}
        self.cards: dict[str, ttk.Frame] = {}
        self.colour_all = False
        # what a chart click highlights besides the table rows: a key per clicked item, and its zones
        self.picked: dict[tuple, list[dict]] = {}
        self.charts: list[dict] = []
        self._targets: dict[str, tuple] = {}
        self._draw_pending = False
        self._selected_guids: set[str] = set()
        self._highlight_key = None
        self.sort_column, self.sort_reverse = "area", True
        self._build_body()
        self.window.add_button("Export...", self.export)
        self.window.add_button("Select in Archicad", self.select_highlighted)
        self.window.add_button("Clear", self.clear)
        self.window.add_button("Colour by parameter", self.colour_by_parameter)
        self.window.add_button("Refresh", self.refresh, primary=True)

    # ---- layout ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)
        family = tkfont.nametofont("TkDefaultFont").actual("family")
        self.window.style.configure("Card.TFrame", relief="solid", borderwidth=1)
        self.window.style.configure("CardBig.TLabel", font=(family, 16, "bold"))
        self.window.style.configure("CardTitle.TLabel", font=(family, 10, "bold"))
        self.window.style.configure("CardSelected.TLabel", foreground="#0b57d0", font=(family, 10, "bold underline"))

        zones = ttk.LabelFrame(body, text="Zones", padding=(10, 6))
        zones.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="project")
        ttk.Radiobutton(zones, text="All zones", value="project", variable=self.scope,
                        command=self.window.guarded(self.refresh)).grid(row=0, column=0, sticky="w")
        ttk.Radiobutton(zones, text="Selected zones", value="selection", variable=self.scope,
                        command=self.window.guarded(self.refresh)).grid(row=0, column=1, sticky="w", padx=(10, 0))
        ttk.Label(zones, text="Area:").grid(row=0, column=2, sticky="w", padx=(24, 0))
        self.area_kind = tk.StringVar(value=AREAS[0][0])
        area = ttk.Combobox(zones, textvariable=self.area_kind, values=[label for label, _ in AREAS], width=34,
                            state="readonly")
        area.grid(row=0, column=3, sticky="w", padx=(6, 0))
        area.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.update_view)())
        ttk.Label(zones, text="Unit:").grid(row=0, column=4, sticky="w", padx=(18, 0))
        self.unit = tk.StringVar(value=UNITS[0][0])
        unit = ttk.Combobox(zones, textvariable=self.unit, values=[u for u, _ in UNITS], width=5, state="readonly")
        unit.grid(row=0, column=5, sticky="w", padx=(6, 0))
        unit.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.update_view)())

        grouping = ttk.LabelFrame(body, text="Grouping", padding=(10, 6))
        grouping.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        ttk.Label(grouping, text="Group by:").grid(row=0, column=0, sticky="w")
        self.group_by = tk.StringVar(value="Zone category")
        self.group_box = ttk.Combobox(grouping, textvariable=self.group_by, width=34, state="readonly")
        self.group_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
        self.group_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.metrics_changed)())
        ttk.Label(grouping, text="Colour by:").grid(row=0, column=2, sticky="w", padx=(18, 0))
        self.colour_by = tk.StringVar(value="Zone category")
        self.colour_box = ttk.Combobox(grouping, textvariable=self.colour_by, width=34, state="readonly")
        self.colour_box.grid(row=0, column=3, sticky="w", padx=(6, 0))
        self.colour_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.metrics_changed)())

        self.filters: list[dict] = []
        for index in range(2):
            ttk.Label(grouping, text=f"Filter {index + 1}:").grid(row=1 + index, column=0, sticky="w", pady=(6, 0))
            metric = tk.StringVar(value=NO_FILTER)
            box = ttk.Combobox(grouping, textvariable=metric, width=34, state="readonly")
            box.grid(row=1 + index, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
            button = ttk.Menubutton(grouping, text="All values", width=30)
            button.grid(row=1 + index, column=2, columnspan=2, sticky="w", padx=(18, 0), pady=(6, 0))
            menu = tk.Menu(button, tearoff=False)
            button["menu"] = menu
            entry = {"metric": metric, "box": box, "button": button, "menu": menu, "values": {}}
            box.bind("<<ComboboxSelected>>", lambda _e, f=entry: self.window.guarded(lambda: self.filter_metric_changed(f))())
            self.filters.append(entry)

        cards = ttk.Frame(body, padding=(0, 10, 0, 6))
        cards.grid(row=2, column=0, sticky="ew")
        cards.columnconfigure(0, weight=1)
        summary = ttk.Frame(cards)
        summary.grid(row=0, column=0, sticky="ew")
        self.summary_labels: dict[str, tuple[ttk.Label, ttk.Label]] = {}
        for column, (key, caption) in enumerate([("total", "total area"), ("count", "zones"),
                                                 ("largest", "largest group"), ("smallest", "smallest group")]):
            cell = ttk.Frame(summary, style="Card.TFrame", padding=(10, 6))
            cell.grid(row=0, column=column, sticky="nsew", padx=(0 if column == 0 else 8, 0))
            summary.columnconfigure(column, weight=1, uniform="summary")
            value = ttk.Label(cell, text="–", style="CardBig.TLabel")
            value.grid(row=0, column=0, sticky="w")
            detail = ttk.Label(cell, text=caption, style="Muted.TLabel")
            detail.grid(row=1, column=0, sticky="w")
            self.summary_labels[key] = (value, detail)
        for key in ("largest", "smallest"):
            for widget in (self.summary_labels[key][0], self.summary_labels[key][0].master, self.summary_labels[key][1]):
                widget.bind("<Button-1>", lambda e, k=key: self.window.guarded(lambda: self._click_summary(k, e))())
        self.group_cards = ttk.Frame(cards)
        self.group_cards.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.group_cards.bind("<Configure>", lambda _e: self._place_cards())
        self._card_columns = 0

        self.notebook = ttk.Notebook(body)
        self.notebook.grid(row=3, column=0, sticky="nsew")
        table = ttk.Frame(self.notebook)
        table.columnconfigure(0, weight=1)
        table.rowconfigure(0, weight=1)
        self.notebook.add(table, text="Table")
        self.tree = make_tree(table, [
            ("area", "Area", 120, "e"), ("count", "Zones", 60, "e"), ("share", "Share", 70, "e"),
            ("story", "Story", 120, "w"),
        ], height=10, show_tree=True)
        self.tree.column("#0", width=280, stretch=True)
        for column in ("#0", "area", "count", "share", "story"):
            self.tree.heading(column, command=lambda c=column: self.window.guarded(lambda: self.sort_by(c))())
        self.tree.container.grid(row=0, column=0, sticky="nsew")
        self._build_charts(family)
        self.tree.tag_configure("total", font=(family, 10, "bold"))
        self.tree.tag_configure("missing", foreground="#a15c00")
        self.tree.bind("<Button-1>", self._on_tree_click)
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self.selection_changed)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- charts ----

    def _build_charts(self, family: str) -> None:
        root = self.window.root
        self.chart_font = tkfont.Font(root=root, family=family, size=9)
        self.chart_bold = tkfont.Font(root=root, family=family, size=9, weight="bold")
        self.chart_big = tkfont.Font(root=root, family=family, size=13, weight="bold")
        for title, draw in (("Share", self._draw_donut), ("Area per group", self._draw_bars),
                            ("Per story", self._draw_stories), ("Treemap", self._draw_treemap)):
            canvas = tk.Canvas(self.notebook, background=CHART_BACKGROUND, highlightthickness=0, height=self._px(260))
            self.notebook.add(canvas, text=title)
            # redrawn when the window is resized
            canvas.bind("<Configure>", lambda _e: self._schedule_draw())
            canvas.tag_bind("hit", "<Button-1>", self._chart_click)
            canvas.tag_bind("hit", "<Enter>", self._chart_enter)
            canvas.tag_bind("hit", "<Leave>", lambda e: e.widget.itemconfigure("info", text=""))
            self.charts.append({"canvas": canvas, "draw": draw})
        self.notebook.bind("<<NotebookTabChanged>>", lambda _e: self._schedule_draw())

    def _px(self, size: float) -> float:
        """ Pixels of a 96 dpi screen on this one, so the charts keep their size on high-DPI screens. """
        return size * self.window.root.winfo_fpixels("1i") / 96

    def _group_metric(self) -> dict:
        return self._metric(self.group_by.get()) or BUILT_IN_METRICS[2]

    def _colour_metric(self) -> dict:
        return self._metric(self.colour_by.get()) or self._group_metric()

    def _schedule_draw(self) -> None:
        if not self._draw_pending:
            self._draw_pending = True
            self.window.root.after_idle(self._draw_current)

    def _draw_current(self) -> None:
        """ Draws the chart of the open tab; the others are drawn when they are opened. """
        self._draw_pending = False
        try:
            index = self.notebook.index("current")
        except tk.TclError:
            return
        if index == 0:
            return
        canvas, draw = self.charts[index - 1]["canvas"], self.charts[index - 1]["draw"]
        canvas.delete("all")
        self._targets = {}
        width, height = canvas.winfo_width(), canvas.winfo_height()
        if width < 60 or height < 60:
            return
        pad = self._px(12)
        info_height = self.chart_font.metrics("linespace") + self._px(4)
        if not self.shown:
            canvas.create_text(width / 2, height / 2, text="There is no zone to show. Press Refresh.",
                               font=self.chart_font, fill=CHART_MUTED)
            return
        self._selected_guids = {z["guid"] for z in self._picked_zones()}
        draw(canvas, pad, pad, width - pad, height - pad - info_height)
        # what the pointer is over
        canvas.create_text(pad, height - pad, anchor="sw", text="", font=self.chart_font, fill=CHART_MUTED, tags=("info",))

    def _target(self, action, info: str) -> tuple[str, str]:
        """ The tags of a clickable chart item: action(event) runs on a click, info shows on hover. """
        tag = f"t{len(self._targets)}"
        self._targets[tag] = (action, info)
        return "hit", tag

    def _chart_click(self, event):
        for tag in event.widget.gettags("current"):
            if tag in self._targets:
                action = self._targets[tag][0]
                self.window.guarded(lambda: action(event))()
                return "break"
        return None

    def _chart_enter(self, event) -> None:
        for tag in event.widget.gettags("current"):
            if tag in self._targets:
                event.widget.itemconfigure("info", text=self._targets[tag][1])
                return

    def _look(self, rgb, zones: list[dict]) -> dict:
        """ The fill and outline of a chart item: as Archicad shows it, the highlighted items in their
        colour with a dark outline and, while something is highlighted, the others faded. """
        selected = [z for z in zones if z["guid"] in self._selected_guids]
        if not self._selected_guids or (selected and len(selected) < len(zones)):
            return {"fill": to_hex(rgb), "outline": CHART_BACKGROUND, "width": 1}
        if selected:
            return {"fill": to_hex(rgb), "outline": CHART_SELECTED, "width": max(2, round(self._px(2)))}
        return {"fill": to_hex(faded(rgb)), "outline": CHART_BACKGROUND, "width": 1}

    def _fit(self, text: str, width: float, font: tkfont.Font) -> str:
        """ The text, cut with an ellipsis to the width. """
        if width <= 0:
            return ""
        if font.measure(text) <= width:
            return text
        low, high = 0, len(text)
        while low < high:
            middle = (low + high + 1) // 2
            if font.measure(text[:middle] + "…") <= width:
                low = middle
            else:
                high = middle - 1
        return text[:low] + "…" if low else ""

    def _title(self, canvas: tk.Canvas, left: float, top: float, right: float, text: str) -> float:
        """ Draws the chart's title and returns the y below it. """
        canvas.create_text(left, top, anchor="nw", text=self._fit(text, right - left, self.chart_bold),
                           font=self.chart_bold, fill=CHART_TEXT)
        return top + self.chart_bold.metrics("linespace") + self._px(8)

    def _area_text(self, area_m2: float | None) -> str:
        unit, factor = self._unit()
        return format_area(area_m2, factor, unit)

    def _legend(self, canvas: tk.Canvas, left: float, top: float, right: float, bottom: float, entries: list[tuple]) -> None:
        """ entries: (colour, text, zones, action, info), one per row, "and N more" when they do not fit. """
        row = max(self.chart_font.metrics("linespace") + self._px(4), self._px(18))
        box = self._px(11)
        for index, (rgb, text, zones, action, info) in enumerate(entries):
            if top + row > bottom or (index < len(entries) - 1 and top + 2 * row > bottom):
                canvas.create_text(left, top + row / 2, anchor="w", text=f"and {len(entries) - index} more",
                                   font=self.chart_font, fill=CHART_MUTED)
                return
            look = self._look(rgb, zones)
            tags = self._target(action, info)
            canvas.create_rectangle(left, top + (row - box) / 2, left + box, top + (row + box) / 2, fill=look["fill"],
                                    outline=look["outline"] if look["width"] > 1 else look["fill"], width=look["width"], tags=tags)
            font = self.chart_bold if look["width"] > 1 else self.chart_font
            canvas.create_text(left + box + self._px(6), top + row / 2, anchor="w", font=font, fill=CHART_TEXT, tags=tags,
                               text=self._fit(text, right - left - box - self._px(6), font))
            top += row

    def _group_entries(self) -> list[tuple]:
        """ The legend of the group-by values, clicked like their table rows. """
        colors = self._colors(self._group_metric()["key"])
        return [(colors.get(g["value"], NO_VALUE_COLOR), f"{g['value']}  {g['share'] * 100:.1f} %", g["zones"],
                 lambda e, v=g["value"]: self.click_group(v, e), self._group_info(g)) for g in self.groups]

    def _colour_entries(self) -> list[tuple]:
        """ The legend of the colour-by values of the listed zones; a click highlights the zones with the value. """
        metric = self._colour_metric()
        colors = self._colors(metric["key"])
        by_value: dict[str, list[dict]] = {}
        for zone in self.shown:
            by_value.setdefault(metric_value(zone, metric["key"]), []).append(zone)
        _label, area_key = self._area()
        entries = []
        for value in sorted(by_value, key=lambda v: value_sort_key(v, self.orders.get(metric["key"]))):
            zones = by_value[value]
            entries.append((colors.get(value, NO_VALUE_COLOR), value, zones,
                            lambda e, v=value, z=zones: self.pick(("colour", v), z, e),
                            f"{metric['label']}: {value}, {self._area_text(totals(zones, area_key)['area'])}, "
                            f"{len(zones)} zone(s)"))
        return entries

    def _group_info(self, group: dict) -> str:
        return (f"{self._group_metric()['label']}: {group['value']}, {self._area_text(group['area'])}, "
                f"{group['share'] * 100:.1f} %, {group['count']} zone(s)")

    def _draw_donut(self, canvas: tk.Canvas, left: float, top: float, right: float, bottom: float) -> None:
        area_label, _area_key = self._area()
        top = self._title(canvas, left, top, right, f"Share of the {area_label.lower()} per {self._group_metric()['label'].lower()}")
        legend_width = min((right - left) / 2, self._px(280))
        size = min(right - left - legend_width - self._px(16), bottom - top)
        colors = self._colors(self._group_metric()["key"])
        if size >= self._px(60):
            radius = size / 2
            cx, cy = left + radius, top + radius
            by_value = {g["value"]: g for g in self.groups}
            slices = donut_slices(self.groups)
            # the highlighted slices last, so their outline is on top
            slices.sort(key=lambda s: self._look(NO_VALUE_COLOR, by_value[s["value"]]["zones"])["width"])
            for piece in slices:
                group = by_value[piece["value"]]
                look = self._look(colors.get(piece["value"], NO_VALUE_COLOR), group["zones"])
                tags = self._target(lambda e, v=piece["value"]: self.click_group(v, e), self._group_info(group))
                if piece["extent"] >= 359.99:
                    canvas.create_oval(cx - radius, cy - radius, cx + radius, cy + radius, tags=tags, **look)
                else:
                    canvas.create_arc(cx - radius, cy - radius, cx + radius, cy + radius, start=piece["start"],
                                      extent=piece["extent"], style="pieslice", tags=tags, **look)
            hole = radius * 0.55
            canvas.create_oval(cx - hole, cy - hole, cx + hole, cy + hole, fill=CHART_BACKGROUND, outline=CHART_BACKGROUND)
            total = totals(self.shown, self._area()[1])
            canvas.create_text(cx, cy, anchor="s", text=self._fit(self._area_text(total["area"]), 2 * hole, self.chart_big),
                               font=self.chart_big, fill=CHART_TEXT)
            canvas.create_text(cx, cy + self._px(2), anchor="n", font=self.chart_font, fill=CHART_MUTED,
                               text=self._fit(f"{total['count']} zones", 2 * hole, self.chart_font))
            if not slices:
                canvas.create_text(cx, cy + radius / 2, text=f"No zone has a {area_label.lower()}.", font=self.chart_font,
                                   fill=CHART_MUTED)
            legend_left = left + size + self._px(16)
        else:
            legend_left = left
        self._legend(canvas, legend_left, top, right, bottom, self._group_entries())

    def _draw_bars(self, canvas: tk.Canvas, left: float, top: float, right: float, bottom: float) -> None:
        area_label, _area_key = self._area()
        group_metric = self._group_metric()
        top = self._title(canvas, left, top, right, f"{area_label} per {group_metric['label'].lower()}, largest first")
        colors = self._colors(group_metric["key"])
        row = max(self.chart_font.metrics("linespace") + self._px(8), self._px(22))
        fit = max(1, int((bottom - top) // row))
        groups = self.groups if len(self.groups) <= fit else self.groups[:max(1, fit - 1)]
        label_width = min((right - left) * 0.35, max(self.chart_font.measure(g["value"]) for g in self.groups) + self._px(8))
        value_width = self.chart_font.measure("000,000.00 ft² · 100.0 %") + self._px(8)
        bar_left = left + label_width
        bar_width = max(self._px(20), right - bar_left - value_width)
        largest = max((g["area"] for g in self.groups), default=0) or 1.0
        for index, group in enumerate(groups):
            y = top + index * row
            look = self._look(colors.get(group["value"], NO_VALUE_COLOR), group["zones"])
            tags = self._target(lambda e, v=group["value"]: self.click_group(v, e), self._group_info(group))
            font = self.chart_bold if look["width"] > 1 else self.chart_font
            canvas.create_text(bar_left - self._px(8), y + row / 2, anchor="e", font=font, fill=CHART_TEXT, tags=tags,
                               text=self._fit(group["value"], label_width - self._px(8), font))
            length = max(1.0, group["area"] / largest * bar_width) if group["area"] > 0 else 0
            if length:
                canvas.create_rectangle(bar_left, y + self._px(3), bar_left + length, y + row - self._px(3), tags=tags, **look)
            text = f"{self._area_text(group['area'])} · {group['share'] * 100:.1f} %" if group["area"] > 0 else "no area"
            canvas.create_text(bar_left + length + self._px(6), y + row / 2, anchor="w", text=text, font=self.chart_font,
                               fill=CHART_MUTED, tags=tags)
        if len(groups) < len(self.groups):
            canvas.create_text(bar_left, top + len(groups) * row + row / 2, anchor="w", font=self.chart_font, fill=CHART_MUTED,
                               text=f"and {len(self.groups) - len(groups)} more groups in the table")

    def _draw_stories(self, canvas: tk.Canvas, left: float, top: float, right: float, bottom: float) -> None:
        area_label, area_key = self._area()
        colour_metric = self._colour_metric()
        top = self._title(canvas, left, top, right, f"{area_label} per story by {colour_metric['label'].lower()}")
        colors = self._colors(colour_metric["key"])
        stack = story_stack(self.shown, area_key, colour_metric["key"], self.orders.get("story"),
                            self.orders.get(colour_metric["key"]))
        # the highest story at the top
        stack.reverse()
        legend_width = min((right - left) * 0.3, self._px(220))
        self._legend(canvas, right - legend_width, top, right, bottom, self._colour_entries())
        right -= legend_width + self._px(16)
        row = min(self._px(28), max(self.chart_font.metrics("linespace") + self._px(4), (bottom - top) / max(1, len(stack))))
        fit = max(1, int((bottom - top) // row))
        stories = stack if len(stack) <= fit else stack[:max(1, fit - 1)]
        label_width = min((right - left) * 0.3, max(self.chart_font.measure(e["story"]) for e in stack) + self._px(8))
        value_width = self.chart_font.measure("000,000.00 ft²") + self._px(8)
        bar_left = left + label_width
        bar_width = max(self._px(20), right - bar_left - value_width)
        largest = max((e["area"] for e in stack), default=0) or 1.0
        for index, entry in enumerate(stories):
            y = top + index * row
            zones = [z for part in entry["parts"] for z in part["zones"]]
            story_tags = self._target(lambda e, s=entry["story"], z=zones: self.pick(("story", s), z, e),
                                      f"Story {entry['story']}: {self._area_text(entry['area'])}, {entry['count']} zone(s)")
            canvas.create_text(bar_left - self._px(8), y + row / 2, anchor="e", font=self.chart_font, fill=CHART_TEXT,
                               tags=story_tags, text=self._fit(entry["story"], label_width - self._px(8), self.chart_font))
            x = bar_left
            for part in entry["parts"]:
                if part["area"] <= 0:
                    continue
                length = part["area"] / largest * bar_width
                look = self._look(colors.get(part["value"], NO_VALUE_COLOR), part["zones"])
                tags = self._target(lambda e, s=entry["story"], v=part["value"], z=part["zones"]: self.pick(("stack", s, v), z, e),
                                    f"{entry['story']}, {colour_metric['label']}: {part['value']}, {self._area_text(part['area'])}, "
                                    f"{part['area'] / entry['area'] * 100:.1f} % of the story, {len(part['zones'])} zone(s)")
                canvas.create_rectangle(x, y + self._px(3), x + length, y + row - self._px(3), tags=tags, **look)
                x += length
            canvas.create_text(x + self._px(6), y + row / 2, anchor="w", font=self.chart_font, fill=CHART_MUTED, tags=story_tags,
                               text=self._area_text(entry["area"]) if entry["area"] > 0 else "no area")
        if len(stories) < len(stack):
            canvas.create_text(bar_left, top + len(stories) * row + row / 2, anchor="w", font=self.chart_font, fill=CHART_MUTED,
                               text=f"and {len(stack) - len(stories)} more stories")

    def _draw_treemap(self, canvas: tk.Canvas, left: float, top: float, right: float, bottom: float) -> None:
        area_label, area_key = self._area()
        group_metric, colour_metric = self._group_metric(), self._colour_metric()
        top = self._title(canvas, left, top, right, f"The zones by {group_metric['label'].lower()}, coloured by "
                                                    f"{colour_metric['label'].lower()}, sized by the {area_label.lower()}")
        group_colors, colors = self._colors(group_metric["key"]), self._colors(colour_metric["key"])
        legend_width = min((right - left) * 0.3, self._px(220))
        self._legend(canvas, right - legend_width, top, right, bottom, self._colour_entries())
        right -= legend_width + self._px(16)
        if right - left < self._px(40) or bottom - top < self._px(40):
            return
        header = self.chart_font.metrics("linespace") + self._px(4)
        rects = squarify([g["area"] for g in self.groups], left, top, right - left, bottom - top)
        for group, (x, y, width, height) in zip(self.groups, rects):
            if width < 1 or height < 1:
                continue
            group_tags = self._target(lambda e, v=group["value"]: self.click_group(v, e), self._group_info(group))
            look = self._look(group_colors.get(group["value"], NO_VALUE_COLOR), group["zones"])
            has_header = height > 2.5 * header and width > self._px(40)
            inner_top = y + header if has_header else y
            if has_header:
                canvas.create_rectangle(x, y, x + width, inner_top, fill=look["fill"], outline=CHART_BACKGROUND, tags=group_tags)
                canvas.create_text(x + self._px(4), y + header / 2, anchor="w", font=self.chart_bold,
                                   fill=text_color(group_colors.get(group["value"], NO_VALUE_COLOR)), tags=group_tags,
                                   text=self._fit(f"{group['value']}  {group['share'] * 100:.1f} %", width - self._px(8), self.chart_bold))
            zones = [z for z in group["zones"] if (zone_area(z, area_key) or 0) > 0]
            tiles = squarify([zone_area(z, area_key) for z in zones], x, inner_top, width, y + height - inner_top)
            for zone, (zx, zy, zw, zh) in zip(zones, tiles):
                if zw <= 0 or zh <= 0:
                    continue
                value = metric_value(zone, colour_metric["key"])
                rgb = colors.get(value, NO_VALUE_COLOR)
                zone_look = self._look(rgb, [zone])
                name = f"{metric_value(zone, 'number')} {metric_value(zone, 'name')}"
                tags = self._target(lambda e, g=zone["guid"]: self.click_row(f"z{g}", e),
                                    f"{name}, {metric_value(zone, 'story')}: {self._area_text(zone_area(zone, area_key))}, "
                                    f"{colour_metric['label']}: {value}")
                canvas.create_rectangle(zx, zy, zx + zw, zy + zh, tags=tags, **zone_look)
                if zw > self._px(36) and zh > header:
                    canvas.create_text(zx + self._px(3), zy + self._px(2), anchor="nw", font=self.chart_font, tags=tags,
                                       fill=text_color(rgb) if zone_look["fill"] == to_hex(rgb) else CHART_MUTED,
                                       text=self._fit(name, zw - self._px(6), self.chart_font))
            # the group's border over its zones
            canvas.create_rectangle(x, y, x + width, y + height, outline=CHART_TEXT, width=1)

    # ---- metrics ----

    def _metric(self, label: str) -> dict | None:
        return next((m for m in self.metrics if m["label"] == label), None)

    def _area(self) -> tuple[str, str]:
        label = self.area_kind.get()
        return label, dict(AREAS)[label]

    def _unit(self) -> tuple[str, float]:
        unit = self.unit.get()
        return unit, dict(UNITS)[unit]

    def _read_metric_list(self) -> None:
        """ The metrics of the pickers: the built-in ones, every classification system and every custom property. """
        metrics = list(BUILT_IN_METRICS)
        try:
            systems = run_command("GetAllClassificationSystems").get("classificationSystems", [])
        except ArchicadError as e:
            print(f"Cannot read the classification systems: {e}")
            systems = []
        for system in systems:
            if "classificationSystemId" not in system:
                continue
            name = f"{system.get('name', '')} {system.get('version', '')}".strip()
            metrics.append({"key": "classification:" + system["classificationSystemId"]["guid"],
                            "label": f"Classification: {name}", "kind": "classification",
                            "id": system["classificationSystemId"]})
        try:
            properties = run_tapir("GetAllProperties").get("properties", [])
        except ArchicadError as e:
            print(f"Cannot read the properties: {e}")
            properties = []
        custom = sorted((p for p in properties if "propertyId" in p and p.get("propertyType") == "Custom"),
                        key=lambda p: (p.get("propertyGroupName", "").lower(), p.get("propertyName", "").lower()))
        for prop in custom:
            metrics.append({"key": "property:" + prop["propertyId"]["guid"],
                            "label": f"Property: {prop.get('propertyGroupName', '')} / {prop.get('propertyName', '')}",
                            "kind": "property", "id": prop["propertyId"]})
        seen: dict[str, int] = {}
        for metric in metrics:
            label = metric["label"]
            seen[label] = seen.get(label, 0) + 1
            if seen[label] > 1:
                metric["label"] = f"{label} ({seen[label]})"
        self.metrics = metrics
        labels = [m["label"] for m in metrics]
        self.group_box.configure(values=labels)
        self.colour_box.configure(values=labels)
        for variable in (self.group_by, self.colour_by):
            if variable.get() not in labels:
                variable.set("Zone category")
        for entry in self.filters:
            entry["box"].configure(values=[NO_FILTER] + labels)
            if entry["metric"].get() not in labels:
                entry["metric"].set(NO_FILTER)

    def _ensure(self, keys) -> None:
        """ Reads the values of the property and classification metrics that are not read yet. """
        missing = [m for m in self.metrics if m["key"] in set(keys) and m["key"] not in self.loaded]
        if not missing or not self.zones:
            self.loaded |= {m["key"] for m in missing}
            return
        with self.window.busy("Reading the zones' values..."):
            properties = [m for m in missing if m["kind"] == "property"]
            if properties:
                self._read_properties(properties)
            for metric in missing:
                if metric["kind"] == "classification":
                    self._read_classification(metric)
        self.loaded |= {m["key"] for m in missing}

    def _read_properties(self, metrics: list[dict]) -> None:
        size = max(1, min(1000, 20000 // len(metrics)))
        done = 0
        for chunk in batched(self.zones, size):
            self.window.set_status(f"Reading the property values... {done} / {len(self.zones)}")
            result = run_tapir("GetPropertyValuesOfElements", {
                "elements": [{"elementId": z["elementId"]} for z in chunk],
                "properties": [{"propertyId": m["id"]} for m in metrics]}).get("propertyValuesForElements", [])
            for zone, entry in zip(chunk, result):
                items = (entry.get("propertyValues") or []) if "error" not in entry else []
                for index, metric in enumerate(metrics):
                    item = items[index] if index < len(items) else {}
                    value = (item.get("propertyValue") or {}).get("value") if "propertyValue" in item else None
                    zone["values"][metric["key"]] = clean_value(value)
            done += len(chunk)
        for zone in self.zones:
            for metric in metrics:
                zone["values"].setdefault(metric["key"], NO_VALUE)

    def _read_classification(self, metric: dict) -> None:
        items: dict[str, str] = {}

        def collect(classification_items: list[dict]) -> None:
            for wrapper in classification_items:
                item = wrapper.get("classificationItem", {})
                if "classificationItemId" in item:
                    items[item["classificationItemId"]["guid"].lower()] = \
                        f"{item.get('id', '')} {item.get('name', '')}".strip() or item["classificationItemId"]["guid"]
                collect(item.get("children", []))

        tree = run_command("GetAllClassificationsInSystem", {"classificationSystemId": metric["id"]})
        collect(tree.get("classificationItems", []))
        done = 0
        for chunk in batched(self.zones):
            self.window.set_status(f"Reading the classifications... {done} / {len(self.zones)}")
            result = run_tapir("GetClassificationsOfElements", {
                "elements": [{"elementId": z["elementId"]} for z in chunk],
                "classificationSystemIds": [{"classificationSystemId": metric["id"]}],
            }).get("elementClassifications", [])
            for zone, entry in zip(chunk, result):
                classification = (entry.get("classificationIds") or [{"error": None}])[0]
                # the Add-On returns the ids directly, the schema wraps them in classificationId
                classification = classification.get("classificationId", classification)
                guid = (classification.get("classificationItemId") or {}).get("guid", "") if "error" not in classification else ""
                # unclassified: no item, or the null GUID
                zone["values"][metric["key"]] = items.get(guid.lower(), guid) if guid.strip("0-") else NO_VALUE
            done += len(chunk)
        for zone in self.zones:
            zone["values"].setdefault(metric["key"], NO_VALUE)

    def _colors(self, key: str) -> dict[str, tuple[int, int, int]]:
        """ The colours of a metric's values, from all the zones read, so a filter does not change them. """
        if key not in self._color_cache:
            self._color_cache[key] = assign_colors((metric_value(z, key) for z in self.zones), self.orders.get(key))
        return self._color_cache[key]

    def _swatch(self, rgb) -> tk.PhotoImage:
        size = max(10, round(12 * self.window.root.winfo_fpixels("1i") / 96))
        key = (tuple(rgb), size)
        if key not in self.swatches:
            image = tk.PhotoImage(master=self.window.root, width=size, height=size)
            image.put(to_hex(rgb), to=(0, 0, size, size))
            self.swatches[key] = image
        return self.swatches[key]

    # ---- reading ----

    def refresh(self) -> None:
        scope = self.scope.get()
        with self.window.busy("Reading the zones..."):
            # the properties and classification systems may have changed since the last Refresh
            self._read_metric_list()
            elements = get_elements(scope, ["Zone"])
            zones = []
            self.orders = {"story": {}}
            if elements:
                details = get_details_of_elements(elements, ["floorIndex", "details"])
                stories = get_story_names()
                categories = self._category_names()
                quantities: list[dict] = []
                for chunk in batched(elements):
                    self.window.set_status(f"Reading the areas... {len(quantities)} / {len(elements)}")
                    result = run_tapir("GetQuantitiesOfElements", {"elements": chunk}).get("quantitiesOfElements", [])
                    quantities += [(item.get("quantities") or {}) if "error" not in item else {} for item in result]
                if len(details) != len(elements) or len(quantities) != len(elements):
                    raise ArchicadError("Archicad returned a different number of results than zones asked for.")
                for element, detail, quantity in zip(elements, details, quantities):
                    info = (detail.get("details") or {}) if "error" not in detail else {}
                    floor = detail.get("floorIndex")
                    category = ((info.get("categoryAttributeId") or {}).get("guid") or "").lower()
                    zones.append({
                        "elementId": element["elementId"],
                        "guid": element["elementId"]["guid"],
                        "floorIndex": floor,
                        "areas": {key: quantity.get(key) for _label, key in AREAS},
                        "values": {
                            "name": clean_value(info.get("name")),
                            "number": clean_value(info.get("numberStr")),
                            "category": categories.get(category, NO_VALUE),
                            "story": clean_value(stories.get(floor, "" if floor is None else str(floor))),
                        },
                    })
                for floor, name in sorted(stories.items()):
                    self.orders["story"].setdefault(clean_value(name), floor)
        self.zones = zones
        self.loaded = {m["key"] for m in BUILT_IN_METRICS}
        self._color_cache = {}
        if not zones:
            for entry in self.filters:
                self._fill_filter_menu(entry)
            self.update_view()
            self.window.set_status("There is no zone to show." if scope == "project"
                                   else "Select zones in Archicad first, then press Refresh.", "warning")
            return
        self.metrics_changed()

    def _category_names(self) -> dict[str, str]:
        try:
            attributes = run_tapir("GetAttributesByType", {"attributeType": "ZoneCategory"}).get("attributes", [])
        except ArchicadError as e:
            print(f"Cannot read the zone categories: {e}")
            return {}
        return {a["attributeId"]["guid"].lower(): clean_value(a.get("name")) for a in attributes if "attributeId" in a}

    # ---- filters ----

    def filter_metric_changed(self, entry: dict) -> None:
        metric = self._metric(entry["metric"].get())
        if metric:
            self._ensure([metric["key"]])
        entry["values"] = {}
        self._fill_filter_menu(entry)
        self.update_view()

    def _fill_filter_menu(self, entry: dict) -> None:
        """ One check item per value of the filter's metric; the ticks of values still there are kept. """
        menu = entry["menu"]
        menu.delete(0, "end")
        metric = self._metric(entry["metric"].get())
        old = {value: var.get() for value, var in entry["values"].items()}
        entry["values"] = {}
        if metric is None or not self.zones:
            entry["button"].configure(text="All values")
            entry["button"].state(["disabled"])
            return
        entry["button"].state(["!disabled"])
        values = sorted({metric_value(z, metric["key"]) for z in self.zones},
                        key=lambda v: value_sort_key(v, self.orders.get(metric["key"])))
        menu.add_command(label="All values", command=self.window.guarded(lambda: self._clear_filter(entry)))
        menu.add_separator()
        for value in values:
            variable = tk.BooleanVar(master=self.window.root, value=old.get(value, False))
            entry["values"][value] = variable
            menu.add_checkbutton(label=value, variable=variable, command=self.window.guarded(self.update_view))
        self._label_filter(entry)

    def _clear_filter(self, entry: dict) -> None:
        for variable in entry["values"].values():
            variable.set(False)
        self.update_view()

    def _label_filter(self, entry: dict) -> None:
        chosen = [value for value, variable in entry["values"].items() if variable.get()]
        if not chosen:
            text = "All values"
        elif len(chosen) == 1:
            text = f"Only {chosen[0]}"
        else:
            text = f"{len(chosen)} of {len(entry['values'])} values"
        entry["button"].configure(text=text if len(text) <= 40 else text[:39] + "…")

    def _active_filters(self) -> dict[str, set[str]]:
        filters: dict[str, set[str]] = {}
        for entry in self.filters:
            metric = self._metric(entry["metric"].get())
            chosen = {value for value, variable in entry["values"].items() if variable.get()}
            if metric and chosen:
                # two filters on the same metric: a zone must pass both
                filters[metric["key"]] = filters[metric["key"]] & chosen if metric["key"] in filters else chosen
        return filters

    # ---- the view ----

    def metrics_changed(self) -> None:
        keys = [m["key"] for m in (self._metric(self.group_by.get()), self._metric(self.colour_by.get())) if m]
        keys += [m["key"] for m in (self._metric(e["metric"].get()) for e in self.filters) if m]
        self._ensure(keys)
        for entry in self.filters:
            self._fill_filter_menu(entry)
        self.update_view()

    def update_view(self) -> None:
        for entry in self.filters:
            self._label_filter(entry)
        group_metric = self._metric(self.group_by.get()) or BUILT_IN_METRICS[2]
        area_label, area_key = self._area()
        filters = self._active_filters()
        self.shown = filter_zones(self.zones, filters)
        # the table is filled again, so its selection goes; what the charts highlighted goes with it
        self.picked = {}
        self.groups = group_zones(self.shown, group_metric["key"], area_key, self.orders.get(group_metric["key"]))
        self._fill_summary(area_label, area_key)
        self._fill_cards(group_metric)
        self._fill_table(group_metric)
        self._apply_highlight(force=True)
        if self.zones:
            total = totals(self.shown, area_key)
            unit, factor = self._unit()
            filtered = f" of {len(self.zones)}" if filters else ""
            missing = f" {total['missing']} zone(s) have no {area_label.lower()}." if total["missing"] else ""
            self.window.set_status(
                f"{len(self.shown)}{filtered} zone(s), {format_area(total['area'], factor, unit)} "
                f"{area_label.lower()} in {len(self.groups)} group(s).{missing}", "warning" if missing else "ok")

    def _fill_summary(self, area_label: str, area_key: str) -> None:
        unit, factor = self._unit()
        total = totals(self.shown, area_key)
        largest, smallest = largest_and_smallest(self.groups)
        has_zones = bool(self.shown)
        self.summary_labels["total"][0].configure(text=format_area(total["area"], factor, unit) if has_zones else "–")
        self.summary_labels["total"][1].configure(text=f"total {area_label.lower()}")
        self.summary_labels["count"][0].configure(text=str(total["count"]) if has_zones else "–")
        self.summary_labels["count"][1].configure(
            text=f"zones, {total['missing']} without area" if total["missing"] else "zones")
        for key, group, caption in (("largest", largest, "largest group"), ("smallest", smallest, "smallest group")):
            if group is None:
                self.summary_labels[key][0].configure(text="–")
                self.summary_labels[key][1].configure(text=caption)
                continue
            self.summary_labels[key][0].configure(text=format_area(group["area"], factor, unit))
            self.summary_labels[key][1].configure(text=f"{caption}: {group['value']} ({group['share'] * 100:.1f} %)")

    def _fill_cards(self, group_metric: dict) -> None:
        for child in self.group_cards.winfo_children():
            child.destroy()
        self.cards = {}
        unit, factor = self._unit()
        colors = self._colors(group_metric["key"])
        for group in self.groups[:MAX_CARDS]:
            card = ttk.Frame(self.group_cards, style="Card.TFrame", padding=(0, 0, 8, 4))
            card.columnconfigure(1, weight=1)
            stripe = tk.Frame(card, width=8, background=to_hex(colors.get(group["value"], NO_VALUE_COLOR)))
            stripe.grid(row=0, column=0, rowspan=2, sticky="ns", padx=(0, 8))
            title = ttk.Label(card, text=group["value"] if len(group["value"]) <= 28 else group["value"][:27] + "…",
                              style="CardTitle.TLabel")
            title.grid(row=0, column=1, sticky="w", pady=(4, 0))
            detail = ttk.Label(card, style="Muted.TLabel",
                               text=f"{format_area(group['area'], factor, unit)} · {group['share'] * 100:.1f} % · "
                                    f"{group['count']} zone{'s' if group['count'] != 1 else ''}")
            detail.grid(row=1, column=1, sticky="w")
            for widget in (card, stripe, title, detail):
                widget.bind("<Button-1>", lambda e, v=group["value"]: self.window.guarded(lambda: self.click_group(v, e))())
            card.title = title
            self.cards[group["value"]] = card
        if len(self.groups) > MAX_CARDS:
            more = ttk.Label(self.group_cards, style="Muted.TLabel",
                             text=f"and {len(self.groups) - MAX_CARDS} more groups in the table")
            self.cards["\0more"] = more
        self._card_columns = 0
        self._place_cards()

    def _place_cards(self) -> None:
        width = self.group_cards.winfo_width()
        columns = max(1, min(4, width // max(1, round(220 * self.window.root.winfo_fpixels("1i") / 96)))) if width > 1 else 4
        if columns == self._card_columns and self.group_cards.grid_slaves():
            return
        self._card_columns = columns
        for column in range(4):
            self.group_cards.columnconfigure(column, weight=1 if column < columns else 0, uniform="cards" if column < columns else "")
        for index, card in enumerate(self.cards.values()):
            card.grid(row=index // columns, column=index % columns, sticky="nsew", padx=(0, 6), pady=(0, 6))

    def _fill_table(self, group_metric: dict) -> None:
        self.tree.delete(*self.tree.get_children())
        self.row_zones, self.group_rows = {}, {}
        unit, factor = self._unit()
        _area_label, area_key = self._area()
        self.tree.heading("#0", text=group_metric["label"])
        self.tree.heading("area", text=f"Area ({unit})")
        colors = self._colors(group_metric["key"])
        colour_metric = self._metric(self.colour_by.get()) or group_metric
        zone_colors = self._colors(colour_metric["key"])
        for index, group in enumerate(self._sorted_groups()):
            iid = f"g{index}"
            self.group_rows[group["value"]] = iid
            self.row_zones[iid] = group["zones"]
            self.tree.insert("", "end", iid=iid, text=group["value"], image=self._swatch(colors.get(group["value"], NO_VALUE_COLOR)),
                             tags=("missing",) if group["missing"] == group["count"] else (),
                             values=(f"{group['area'] / factor:,.2f}", group["count"], f"{group['share'] * 100:.1f} %", ""))
            for zone in self._sorted_zones(group["zones"], area_key):
                zone_iid = f"z{zone['guid']}"
                self.row_zones[zone_iid] = [zone]
                area = in_unit(zone_area(zone, area_key), factor)
                label = f"{metric_value(zone, 'number')} {metric_value(zone, 'name')}"
                self.tree.insert(iid, "end", iid=zone_iid, text=label,
                                 image=self._swatch(zone_colors.get(metric_value(zone, colour_metric["key"]), NO_VALUE_COLOR)),
                                 tags=("missing",) if area is None else (),
                                 values=(f"{area:,.2f}" if area is not None else "no area", "", "", metric_value(zone, "story")))
        if self.groups:
            total = totals(self.shown, area_key)
            self.row_zones["total"] = self.shown
            self.tree.insert("", "end", iid="total", text="Total", tags=("total",),
                             values=(f"{total['area'] / factor:,.2f}", total["count"], "100.0 %" if total["area"] > 0 else "", ""))

    def _sorted_groups(self) -> list[dict]:
        column, reverse = self.sort_column, self.sort_reverse
        group_metric = self._metric(self.group_by.get()) or BUILT_IN_METRICS[2]
        order = self.orders.get(group_metric["key"])
        if column in ("area", "share"):
            key = lambda g: g["area"]  # noqa: E731
        elif column == "count":
            key = lambda g: g["count"]  # noqa: E731
        else:
            key = lambda g: value_sort_key(g["value"], order)  # noqa: E731
        return sorted(self.groups, key=key, reverse=reverse)

    def _sorted_zones(self, zones: list[dict], area_key: str) -> list[dict]:
        column, reverse = self.sort_column, self.sort_reverse
        if column in ("area", "share", "count"):
            key = lambda z: zone_area(z, area_key) if zone_area(z, area_key) is not None else -1.0  # noqa: E731
        elif column == "story":
            key = lambda z: (z["floorIndex"] if z["floorIndex"] is not None else 0, natural_key(metric_value(z, "number")))  # noqa: E731
        else:
            key = lambda z: (natural_key(metric_value(z, "number")), natural_key(metric_value(z, "name")))  # noqa: E731
        return sorted(zones, key=key, reverse=reverse)

    def sort_by(self, column: str) -> None:
        if column == self.sort_column:
            self.sort_reverse = not self.sort_reverse
        else:
            self.sort_column, self.sort_reverse = column, column in ("area", "count", "share")
        selected = self.tree.selection()
        opened = [iid for iid in self.tree.get_children() if self.tree.item(iid, "open")]
        opened_values = {self.tree.item(iid, "text") for iid in opened}
        selected_values = {self.tree.item(iid, "text") for iid in selected if iid.startswith("g")}
        selected_zones = [iid for iid in selected if iid.startswith("z") or iid == "total"]
        self._fill_table(self._metric(self.group_by.get()) or BUILT_IN_METRICS[2])
        for value, iid in self.group_rows.items():
            if value in opened_values:
                self.tree.item(iid, open=True)
        rows = [iid for value, iid in self.group_rows.items() if value in selected_values]
        rows += [iid for iid in selected_zones if self.tree.exists(iid)]
        self.tree.selection_set(rows)

    # ---- highlight ----

    def _on_tree_click(self, event):
        row = self.tree.identify_row(event.y)
        if not row or self.tree.identify_region(event.x, event.y) not in ("tree", "cell"):
            return None
        if "indicator" in str(self.tree.identify_element(event.x, event.y)):
            return None
        if is_additive_click(event):
            if row in self.tree.selection():
                self.tree.selection_remove(row)
            else:
                self.tree.selection_add(row)
            return "break"
        if self.picked:
            # a plain click replaces what the charts highlighted
            self.picked = {}
            if self.tree.selection() == (row,):
                self.window.guarded(self.selection_changed)()
                return "break"
            return None
        if self.tree.selection() == (row,):
            # clicking the highlighted row again removes the highlight
            self.tree.selection_set(())
            return "break"
        return None

    def click_group(self, value: str, event=None) -> None:
        self.click_row(self.group_rows.get(value), event)

    def click_row(self, iid: str | None, event=None) -> None:
        """ A card or a chart item standing for a table row: highlights it like a click on the row. """
        if iid is None or not self.tree.exists(iid):
            return
        if is_additive_click(event):
            if iid in self.tree.selection():
                self.tree.selection_remove(iid)
            else:
                self.tree.selection_add(iid)
        elif self.tree.selection() == (iid,) and not self.picked:
            self.tree.selection_set(())
        else:
            self.picked = {}
            if self.tree.selection() == (iid,):
                self.selection_changed()
            self.tree.selection_set(iid)
        self.tree.see(iid)

    def pick(self, key: tuple, zones: list[dict], event=None) -> None:
        """ A chart item that is no table row (a part of a story's bar, a colour in a legend):
        highlights its zones; Ctrl/Cmd-click adds or removes it, clicking it again clears it. """
        if not zones:
            return
        if is_additive_click(event):
            if key in self.picked:
                del self.picked[key]
            else:
                self.picked[key] = zones
        elif list(self.picked) == [key] and not self.tree.selection():
            self.picked = {}
        else:
            self.picked = {key: zones}
            self.tree.selection_set(())
        self.selection_changed()

    def _click_summary(self, key: str, event) -> None:
        largest, smallest = largest_and_smallest(self.groups)
        group = largest if key == "largest" else smallest
        if group is not None:
            self.click_group(group["value"], event)

    def selection_changed(self) -> None:
        selected = set(self.tree.selection())
        for value, card in self.cards.items():
            if hasattr(card, "title"):
                card.title.configure(style="CardSelected.TLabel" if self.group_rows.get(value) in selected else "CardTitle.TLabel")
        self._apply_highlight()

    def _picked_zones(self) -> list[dict]:
        """ The zones of the selected table rows and of the clicked chart items. """
        seen, zones = set(), []
        for members in [self.row_zones.get(row, []) for row in self.tree.selection()] + list(self.picked.values()):
            for zone in members:
                if zone["guid"] not in seen:
                    seen.add(zone["guid"])
                    zones.append(zone)
        return zones

    def _highlighted_zones(self) -> list[dict]:
        zones = self._picked_zones()
        if not zones and self.colour_all:
            return list(self.shown)
        return zones

    def _apply_highlight(self, force: bool = False) -> None:
        self._schedule_draw()
        zones = self._highlighted_zones()
        colour_metric = self._metric(self.colour_by.get()) or BUILT_IN_METRICS[2]
        key = (tuple(z["guid"] for z in zones), colour_metric["key"])
        if not force and key == self._highlight_key:
            return
        self._highlight_key = key
        if not zones:
            if self.zones:
                run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
            return
        colors = self._colors(colour_metric["key"])
        run_tapir("HighlightElements", {
            "elements": [{"elementId": z["elementId"]} for z in zones],
            "highlightedColors": [list(colors.get(metric_value(z, colour_metric["key"]), NO_VALUE_COLOR)) + [255]
                                  for z in zones],
            "nonHighlightedColor": DIMMED_COLOR,
        })
        if force:
            return
        unit, factor = self._unit()
        area_label, area_key = self._area()
        self.window.set_status(
            f"Highlighted {len(zones)} zone(s), {format_area(totals(zones, area_key)['area'], factor, unit)} "
            f"{area_label.lower()}, coloured by {colour_metric['label'].lower()}.", "ok")

    def colour_by_parameter(self) -> None:
        if not self.shown:
            self.window.set_status("There is no zone to colour. Press Refresh first.", "warning")
            return
        self.colour_all = True
        self.picked = {}
        self.tree.selection_set(())
        self._apply_highlight(force=True)
        colour_metric = self._metric(self.colour_by.get()) or BUILT_IN_METRICS[2]
        self.window.set_status(f"Coloured {len(self.shown)} zone(s) by {colour_metric['label'].lower()}. "
                               "The zones are not changed; Clear removes the colours.", "ok")

    def clear(self) -> None:
        self.colour_all = False
        self.picked = {}
        self.tree.selection_set(())
        self._apply_highlight(force=True)
        self.window.set_status("The highlight is cleared.")

    def close(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    # ---- selection and export ----

    def select_highlighted(self) -> None:
        zones = self._highlighted_zones() or self.shown
        if not zones:
            self.window.set_status("There is no zone to select. Press Refresh first.", "warning")
            return
        select_elements([{"elementId": z["elementId"]} for z in zones])
        self.window.set_status(f"Selected {len(zones)} zone(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row.startswith("z"):
            zone = self.row_zones[row][0]
            select_elements([{"elementId": zone["elementId"]}])
            self.window.set_status(f"Selected zone {metric_value(zone, 'number')} {metric_value(zone, 'name')}.", "ok")

    def export(self) -> None:
        if not self.shown:
            self.window.set_status("There is no zone to export. Press Refresh first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the zone areas",
                                            initialfile=f"Zone areas {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        self._ensure([m["key"] for m in self.metrics])
        area_label, area_key = self._area()
        unit, factor = self._unit()
        area_header = f"{area_label} ({unit})"
        headers, rows = zone_table(self.shown, self.metrics, area_key, area_header, factor)
        if not path.lower().endswith(".xlsx"):
            write_table(path, headers, rows)
            self.window.set_status(f"Exported {len(rows)} zone(s) to {os.path.basename(path)}.", "ok")
            return
        sheets = [("Zones", headers, rows)]
        group_metric = self._metric(self.group_by.get()) or BUILT_IN_METRICS[2]
        colour_metric = self._metric(self.colour_by.get()) or group_metric
        for metric in ([group_metric] if colour_metric is group_metric else [group_metric, colour_metric]):
            groups = group_zones(self.shown, metric["key"], area_key, self.orders.get(metric["key"]))
            group_headers, group_rows = group_table(groups, metric["label"], area_header, factor)
            sheets.append((f"By {re.sub(r'^(Property|Classification): ', '', metric['label'])}", group_headers, group_rows))
        if colour_metric["key"] != "story":
            # the "Per story" chart: the area per story split by the colour-by value
            stack = story_stack(self.shown, area_key, colour_metric["key"], self.orders.get("story"),
                                self.orders.get(colour_metric["key"]))
            story_headers, story_rows = story_table(stack, colour_metric["label"], area_header, factor,
                                                    self.orders.get(colour_metric["key"]))
            sheets.append((f"Story x {re.sub(r'^(Property|Classification): ', '', colour_metric['label'])}",
                           story_headers, story_rows))
        write_workbook(path, sheets)
        self.window.set_status(f"Exported {len(rows)} zone(s) and {len(sheets) - 1} grouping(s) to "
                               f"{os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.refresh, on_close=self.close)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ZoneAreaDashboard().run()
