# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Property Highlighter

Colors the elements in the model by the value of a chosen property, with a
legend of the values and their element counts. Text values get one color each;
number values can also be grouped into ranges with a color gradient. Select
values in the legend to highlight only those, double-click one to select its
elements in Archicad.
"""

import colorsys
import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import batched, get_elements, run_tapir, select_elements
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Property Highlighter"
DIMMED_COLOR = [200, 200, 200, 160]
NOT_AVAILABLE = "(not available)"
EMPTY = "(empty)"
NOT_A_NUMBER = "(not a number)"
SPECIAL_COLOR = [150, 150, 150, 255]
ALL_TYPES = "All types"
ELEMENT_TYPES = [
    ALL_TYPES, "Wall", "Column", "Beam", "Slab", "Roof", "Shell", "Morph", "Mesh", "Window", "Door", "Skylight",
    "Opening", "Object", "Lamp", "Zone", "CurtainWall", "Stair", "Railing",
]
# Easy to tell apart colors for the first values; more values get generated hues.
DISTINCT_COLORS = [
    (230, 25, 75), (60, 180, 75), (0, 130, 200), (245, 130, 48), (145, 30, 180), (70, 240, 240),
    (240, 50, 230), (210, 245, 60), (250, 190, 212), (0, 128, 128), (220, 190, 255), (170, 110, 40),
    (255, 225, 25), (128, 0, 0), (170, 255, 195), (128, 128, 0), (255, 215, 180), (0, 0, 128),
]
# The ends and middle points of the gradient for the number ranges: blue - green - yellow - red.
GRADIENT = [(44, 123, 182), (171, 221, 164), (255, 255, 191), (253, 174, 97), (215, 25, 28)]


def distinct_color(index: int) -> tuple[int, int, int]:
    if index < len(DISTINCT_COLORS):
        return DISTINCT_COLORS[index]
    hue = (index * 0.618033988749895) % 1.0
    r, g, b = colorsys.hsv_to_rgb(hue, 0.65, 0.9)
    return round(r * 255), round(g * 255), round(b * 255)


def gradient_color(t: float) -> tuple[int, int, int]:
    """ The color at t (0..1) of the gradient. """
    t = min(1.0, max(0.0, t)) * (len(GRADIENT) - 1)
    i = min(int(t), len(GRADIENT) - 2)
    f = t - i
    return tuple(round(a + (b - a) * f) for a, b in zip(GRADIENT[i], GRADIENT[i + 1]))


_NUMBER = re.compile(r"[-+−]?\d[\d.,'\s  ]*")


def parse_number(text: str) -> float | None:
    """ The first number in a displayed value such as "1 234,5 m²" or "1,234.5". """
    match = _NUMBER.search(text)
    if not match:
        return None
    number = re.sub(r"[\s'  ]", "", match.group()).replace("−", "-").rstrip(".,")
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
    """ Sorts "Room 2" before "Room 10". """
    return [(0, float(part), "") if part.isdigit() else (1, 0.0, part.lower()) for part in re.split(r"(\d+)", text)]


def to_hex(rgb) -> str:
    return "#{:02x}{:02x}{:02x}".format(*rgb[:3])


class PropertyHighlighter:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Colors the elements by the value of a property. Pick a property and press Highlight. "
            "Select values in the legend to highlight only those; double-click a value to select its elements.",
            min_width=560, min_height=640)
        self.properties: list[dict] = []
        self.listed_properties: list[dict] = []
        self.property: dict | None = None
        self.values: list[tuple[dict, str]] = []
        # legend: one group per value or range, {"label", "color", "special", "elements"}
        self.groups: list[dict] = []
        self.swatches: dict[str, tk.PhotoImage] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_listed)
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Highlight", self.highlight, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)
        body.rowconfigure(4, weight=2)

        elements = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        elements.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="story")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(elements, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))
        ttk.Label(elements, text="Type:").grid(row=0, column=3, sticky="e", padx=(18, 0))
        self.element_type = tk.StringVar(value=ALL_TYPES)
        ttk.Combobox(elements, textvariable=self.element_type, values=ELEMENT_TYPES, width=14, state="readonly").grid(
            row=0, column=4, sticky="w", padx=(6, 0))
        self.visible_only = tk.BooleanVar(value=True)
        ttk.Checkbutton(elements, text="Only elements on visible layers", variable=self.visible_only).grid(
            row=1, column=0, columnspan=5, sticky="w", pady=(6, 0))

        properties = ttk.LabelFrame(body, text="Property", padding=(10, 6))
        properties.grid(row=1, column=0, sticky="nsew", pady=(8, 0))
        properties.columnconfigure(1, weight=1)
        properties.rowconfigure(1, weight=1)
        ttk.Label(properties, text="Search:").grid(row=0, column=0, sticky="w")
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_: self._list_properties())
        ttk.Entry(properties, textvariable=self.search).grid(row=0, column=1, sticky="ew", padx=(6, 0))
        self.property_tree = make_tree(properties, [("name", "Property", 230, "w"), ("group", "Group", 200, "w")], height=7)
        self.property_tree.configure(selectmode="browse")
        self.property_tree.container.grid(row=1, column=0, columnspan=2, sticky="nsew", pady=(6, 0))
        self.property_tree.bind("<Double-1>", lambda _e: self.window.guarded(self.highlight)())

        coloring = ttk.Frame(body, padding=(0, 8, 0, 0))
        coloring.grid(row=2, column=0, sticky="ew")
        ttk.Label(coloring, text="Colors:").grid(row=0, column=0, sticky="w")
        self.mode = tk.StringVar(value="values")
        ttk.Radiobutton(coloring, text="One per value", value="values", variable=self.mode,
                        command=self._regroup).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Radiobutton(coloring, text="Number ranges:", value="ranges", variable=self.mode,
                        command=self._regroup).grid(row=0, column=2, sticky="w", padx=(10, 0))
        self.range_count = tk.StringVar(value="5")
        ranges = ttk.Spinbox(coloring, textvariable=self.range_count, from_=2, to=12, width=4,
                             command=self._regroup)
        ranges.grid(row=0, column=3, sticky="w", padx=(4, 0))
        ranges.bind("<Return>", lambda _e: (self._regroup(), "break")[1])
        self.wireframe = tk.BooleanVar(value=False)
        ttk.Checkbutton(coloring, text="Wireframe in 3D", variable=self.wireframe,
                        command=lambda: self.window.guarded(self._apply_highlight)()).grid(
            row=0, column=4, sticky="w", padx=(18, 0))

        summary = ttk.Frame(body, padding=(0, 8, 0, 4))
        summary.grid(row=3, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Pick a property and press Highlight.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.legend = make_tree(body, [("count", "Elements", 90, "e")], height=8, show_tree=True)
        self.legend.heading("#0", text="Value", anchor="w")
        self.legend.column("#0", width=380, stretch=True)
        self.legend.container.grid(row=4, column=0, sticky="nsew")
        self.legend.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._apply_highlight)())
        self.legend.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- properties ----

    def load_properties(self) -> None:
        with self.window.busy("Reading the properties..."):
            properties = run_tapir("GetAllProperties").get("properties", [])
        self.properties = sorted(
            (p for p in properties if "propertyId" in p),
            key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                           p.get("propertyName", "").lower()))
        self._list_properties()
        self.window.set_status(f"{len(self.properties)} properties. Pick one and press Highlight.")

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

    # ---- highlighting ----

    def highlight(self) -> None:
        prop = self._chosen_property()
        if prop is None:
            self.window.set_status("Pick a property in the list first.", "warning")
            return
        element_types = None if self.element_type.get() == ALL_TYPES else [self.element_type.get()]
        filters = ["IsVisibleByLayer"] if self.visible_only.get() else []
        with self.window.busy("Reading the elements..."):
            elements = get_elements(self.scope.get(), element_types, filters)
            values: list[str] = []
            for chunk in batched(elements):
                self.window.set_status(f"Reading the values... {len(values)} / {len(elements)}")
                result = run_tapir("GetPropertyValuesOfElements", {
                    "elements": chunk, "properties": [{"propertyId": prop["propertyId"]}]})
                for entry in result.get("propertyValuesForElements", []):
                    item = (entry.get("propertyValues") or [{}])[0]
                    if "propertyValue" not in item:
                        values.append(NOT_AVAILABLE)
                    else:
                        values.append(item["propertyValue"].get("value", "") or EMPTY)

        self.property = prop
        self.values = list(zip(elements, values))
        self.legend.selection_set(())
        self._regroup()

    def _regroup(self) -> None:
        if self.property is None:
            return
        by_label: dict[str, list[dict]] = {}
        if self.mode.get() == "ranges":
            self._group_ranges(by_label)
        else:
            for element, value in self.values:
                by_label.setdefault(value, []).append({"elementId": element["elementId"]})
            special = [label for label in (EMPTY, NOT_AVAILABLE) if label in by_label]
            ordered = sorted((label for label in by_label if label not in special), key=natural_key)
            self.groups = [{"label": label, "color": distinct_color(i), "special": False, "elements": by_label[label]}
                           for i, label in enumerate(ordered)]
            self.groups += [{"label": label, "color": SPECIAL_COLOR, "special": True, "elements": by_label[label]}
                            for label in special]
        self._show_legend()
        self.window.guarded(self._apply_highlight)()

    def _group_ranges(self, by_label: dict) -> None:
        numbers = [(element, parse_number(value) if value not in (EMPTY, NOT_AVAILABLE) else None, value)
                   for element, value in self.values]
        known = [n for _, n, _ in numbers if n is not None]
        try:
            count = max(2, min(12, int(self.range_count.get())))
        except ValueError:
            count = 5
        self.groups = []
        if known:
            low, high = min(known), max(known)
            count = 1 if high == low else count
            step = (high - low) / count
            ranges = [{"label": f"{low + i * step:.6g} – {low + (i + 1) * step:.6g}" if step else f"{low:.6g}",
                       "color": gradient_color(i / max(1, count - 1)), "special": False, "elements": []}
                      for i in range(count)]
            for element, number, _ in numbers:
                if number is not None:
                    index = min(count - 1, int((number - low) / step)) if step else 0
                    ranges[index]["elements"].append({"elementId": element["elementId"]})
            self.groups = [r for r in ranges if r["elements"]]
        for element, number, value in numbers:
            if number is None:
                label = value if value in (EMPTY, NOT_AVAILABLE) else NOT_A_NUMBER
                by_label.setdefault(label, []).append({"elementId": element["elementId"]})
        self.groups += [{"label": label, "color": SPECIAL_COLOR, "special": True, "elements": by_label[label]}
                        for label in (NOT_A_NUMBER, EMPTY, NOT_AVAILABLE) if label in by_label]

    def _swatch(self, color) -> tk.PhotoImage:
        key = to_hex(color)
        if key not in self.swatches:
            image = tk.PhotoImage(master=self.window.root, width=14, height=14)
            image.put("#666666", to=(0, 0, 14, 14))
            image.put(key, to=(1, 1, 13, 13))
            self.swatches[key] = image
        return self.swatches[key]

    def _show_legend(self) -> None:
        self.legend.delete(*self.legend.get_children())
        for index, group in enumerate(self.groups):
            self.legend.insert("", "end", iid=str(index), text=f" {group['label']}", image=self._swatch(group["color"]),
                               values=(len(group["elements"]),))
        total = sum(len(g["elements"]) for g in self.groups)
        colored = sum(len(g["elements"]) for g in self.groups if not g["special"])
        self.count_label.configure(text=str(len(self.groups)))
        name = self.property.get("propertyName", "")
        kind = "range" if self.mode.get() == "ranges" else "value"
        self.summary_label.configure(text=f"different {kind}{'s' if len(self.groups) != 1 else ''} of {name}")
        self.detail_label.configure(text=f"{total} elements, {colored} of them colored")

    def _apply_highlight(self) -> None:
        rows = self.legend.selection()
        groups = [self.groups[int(r)] for r in rows] if rows else self.groups
        elements, colors = [], []
        for group in groups:
            elements += group["elements"]
            colors += [list(group["color"]) + [255]] * len(group["elements"])
        if not elements:
            self.clear_highlight()
            return
        run_tapir("HighlightElements", {
            "elements": elements,
            "highlightedColors": colors,
            "wireframe3D": self.wireframe.get(),
            "nonHighlightedColor": DIMMED_COLOR,
        })
        if rows:
            self.window.set_status(f"Highlighted {len(elements)} element(s) of {len(groups)} selected value(s).", "ok")
        else:
            self.window.set_status(f"Highlighted {len(elements)} element(s).", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})
        self.window.set_status("The highlight is cleared.")

    # ---- selection ----

    def select_listed(self) -> None:
        rows = self.legend.selection()
        groups = [self.groups[int(r)] for r in rows] if rows else self.groups
        elements = [e for g in groups for e in g["elements"]]
        if not elements:
            self.window.set_status("There is nothing to select. Press Highlight first.", "warning")
            return
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.legend.focus()
        if row:
            group = self.groups[int(row)]
            select_elements(group["elements"])
            self.window.set_status(f"Selected the {len(group['elements'])} element(s) of {group['label']}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_properties, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    PropertyHighlighter().run()
