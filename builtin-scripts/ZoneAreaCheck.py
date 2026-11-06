# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Zone Area Check

Compares the measured area of every zone with a target area read from a
property you choose, within a tolerance in percent. Lists the zones that are
over, under and on target, and highlights them in three colors. It changes
nothing in the project apart from the highlight. Select rows to highlight only
those zones, double-click one to select it in Archicad, and export the list to
Excel or CSV.
"""

import datetime
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


class ZoneAreaCheck:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Compares each zone's area with a target area property. Pick the property that holds the target, "
            "set the tolerance and press Check. Over, under and on target zones are highlighted in three colors. "
            "Select rows to highlight only those; double-click a row to select the zone.",
            min_width=640, min_height=680)
        self.properties: list[dict] = []
        self.listed_properties: list[dict] = []
        self.zones: list[dict] = []  # {"elementId", "number", "name", "story", "measured", "target", "difference", "status"}
        self.shown: list[dict] = []
        self.property_name = ""
        self._build_body()
        self.window.add_button("Export...", self.export)
        self.window.add_button("Select in Archicad", self.select_listed)
        self.window.add_button("Clear highlight", self.clear_highlight)
        self.window.add_button("Check", self.check, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)
        body.rowconfigure(4, weight=2)

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

        summary = ttk.Frame(body, padding=(0, 8, 0, 4))
        summary.grid(row=3, column=0, sticky="ew")
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
        ], height=10)
        self.tree.container.grid(row=4, column=0, sticky="nsew")
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
        self.properties = sorted(
            (p for p in properties if "propertyId" in p),
            key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                           p.get("propertyName", "").lower()))
        self._list_properties()
        self.window.set_status(f"{len(self.properties)} properties. Pick the one with the target area and press Check.")

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
                zone["status"]))
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
                 z["status"], z["elementId"]["guid"]]
                for z in self.zones]
        write_table(path, ["Number", "Name", "Story", "Measured m²", "Target m²", "Difference %", "Status", "GUID"],
                    rows, "Zone area check")
        self.window.set_status(f"Exported {len(rows)} zone(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_properties, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ZoneAreaCheck().run()
