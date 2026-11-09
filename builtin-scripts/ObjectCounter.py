# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "quantities-scheduling"
# changes = "files"
# ///

"""
Object Counter

Counts the placed objects, lamps, doors, windows and (from Archicad 30) electrical
devices by library part, grouped by story and by the zone they stand in. The zone
is found from the element's position and the zone polygons of its story. Optionally
it also sums one numeric GDL parameter, for example a fixture's load or its drainage
fixture units. The table can be saved as CSV or Excel. The script only reads the
model; it changes nothing in it (it can select the counted elements).
"""

import datetime
import math
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, run_tapir,
                                select_elements)
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Object Counter"
# element type, check box text, label in the table
TYPES = [
    ("Object", "Objects", "Object"),
    ("Lamp", "Lamps", "Lamp"),
    ("Door", "Doors", "Door"),
    ("Window", "Windows", "Window"),
    ("ElectricalDevice", "Electrical devices (Archicad 30)", "Electrical device"),
]
GDL_TYPES = ["Object", "Lamp", "Door", "Window"]
NO_ZONE = "(no zone)"
UNKNOWN_POSITION = "(position unknown)"
GDL_BATCH_SIZE = 200
NULL_REVISION = "{00000000-0000-0000-0000-000000000000}"


# ---- geometry ----

def _ring(points: list[dict]) -> list[tuple[float, float]]:
    return [(p["x"], p["y"]) for p in points or [] if "x" in p and "y" in p]


def _inside_ring(x: float, y: float, ring: list[tuple[float, float]]) -> bool:
    """ Ray casting; a closing point equal to the first one does no harm. """
    inside = False
    count = len(ring)
    for i in range(count):
        x1, y1 = ring[i]
        x2, y2 = ring[(i + 1) % count]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            inside = not inside
    return inside


def _ring_area(ring: list[tuple[float, float]]) -> float:
    count = len(ring)
    return abs(sum(ring[i][0] * ring[(i + 1) % count][1] - ring[(i + 1) % count][0] * ring[i][1]
                   for i in range(count))) / 2


class Zone:
    """ A zone's outline on the floor plan. Arcs are taken as straight edges. """

    def __init__(self, label: str, outline: list[dict], holes: list[dict]):
        self.label = label
        self.outline = _ring(outline)
        self.holes = [_ring(h.get("polygonOutline")) for h in holes or []]
        self.area = _ring_area(self.outline) - sum(_ring_area(h) for h in self.holes)

    def contains(self, x: float, y: float) -> bool:
        return (len(self.outline) >= 3 and _inside_ring(x, y, self.outline)
                and not any(len(h) >= 3 and _inside_ring(x, y, h) for h in self.holes))


def _number(value) -> float | None:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    return float(value)


class ObjectCounter:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Counts placed objects, lamps, doors, windows and electrical devices by library part, per story "
            "and per zone. Enter a GDL parameter name to sum its value as well. Double-click a row to select "
            "its elements in Archicad.",
            min_width=820, min_height=620)
        self.enabled = {key: tk.BooleanVar(value=True) for key, _, _ in TYPES}
        self.scope = tk.StringVar(value="project")
        self.by_zone = tk.BooleanVar(value=True)
        self.parameter = tk.StringVar()
        self.rows: list[dict] = []
        self.row_ids: dict[str, int] = {}
        self.sum_header = ""
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_chosen)
        self.window.add_button("Save as...", self.export)
        self.window.add_button("Count", self.count, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="What to count", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        for index, (key, text, _) in enumerate(TYPES):
            ttk.Checkbutton(options, text=text, variable=self.enabled[key]).grid(
                row=0, column=index, sticky="w", padx=(0 if index == 0 else 14, 0))
        scope = ttk.Frame(options)
        scope.grid(row=1, column=0, columnspan=len(TYPES), sticky="w", pady=(6, 0))
        ttk.Label(scope, text="Elements:").grid(row=0, column=0, sticky="w")
        for column, (value, text) in enumerate([("project", "Whole project"), ("story", "Current story"),
                                                ("selection", "Selection")], start=1):
            ttk.Radiobutton(scope, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(10, 0))
        ttk.Checkbutton(scope, text="Group by zone", variable=self.by_zone).grid(
            row=0, column=4, sticky="w", padx=(24, 0))
        extra = ttk.Frame(options)
        extra.grid(row=2, column=0, columnspan=len(TYPES), sticky="w", pady=(6, 0))
        ttk.Label(extra, text="Sum the GDL parameter (optional):").grid(row=0, column=0, sticky="w")
        ttk.Entry(extra, textvariable=self.parameter, width=28).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(extra, text="its name or its name in the settings dialog", style="Muted.TLabel").grid(
            row=0, column=2, sticky="w", padx=(8, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("elements", "elements counted"), ("parts", "library parts"),
                                                 ("zones", "zones with elements"), ("sum", "sum")]):
            summary.columnconfigure(column, weight=1)
            value = ttk.Label(summary, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            caption_label = ttk.Label(summary, text=caption, style="Muted.TLabel")
            caption_label.grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value
            if key == "sum":
                self.sum_caption = caption_label

        self.tree = make_tree(body, [
            ("story", "Story", 130, "w"),
            ("zone", "Zone", 170, "w"),
            ("type", "Type", 110, "w"),
            ("part", "Library part", 220, "w"),
            ("count", "Count", 60, "e"),
            ("sum", "Sum", 90, "e"),
        ], height=16)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- reading ----

    def count(self) -> None:
        chosen = [key for key, _, _ in TYPES if self.enabled[key].get()]
        if not chosen:
            self.window.set_status("Choose at least one element type.", "warning")
            return
        scope = self.scope.get()
        parameter = self.parameter.get().strip()
        notes = []

        with self.window.busy("Reading the stories..."):
            story_list = run_tapir("GetStories")
            stories = {s["index"]: s.get("name") or f"Story {s['index']}" for s in story_list.get("stories", [])}
            levels = sorted((s.get("level", 0.0), s["index"]) for s in story_list.get("stories", []))
            current_story = story_list.get("actStory")

            items = []
            gdl_types = [t for t in chosen if t in GDL_TYPES]
            if gdl_types:
                self.window.set_status("Reading the elements...")
                items += self._read_gdl_elements(scope, gdl_types)
            if "ElectricalDevice" in chosen:
                self.window.set_status("Reading the electrical devices...")
                devices, note = self._read_electrical_devices(scope, current_story, levels)
                items += devices
                if note:
                    notes.append(note)

            zones: dict[int, list[Zone]] = {}
            if self.by_zone.get() and items:
                self.window.set_status("Reading the zones...")
                zones = self._read_zones()

            values: dict[str, float | None] = {}
            if parameter and items:
                self.window.set_status(f"Reading the parameter {parameter}...")
                values = self._read_parameter(items, parameter)

            groups: dict[tuple, dict] = {}
            for item in items:
                zone = ""
                if self.by_zone.get():
                    zone = self._zone_of(item, zones.get(item["floorIndex"], []))
                key = (item["floorIndex"], zone, item["type"], item["part"])
                group = groups.setdefault(key, {"elements": [], "sum": 0.0, "valued": 0})
                group["elements"].append({"elementId": item["elementId"]})
                value = values.get(item["elementId"]["guid"])
                if value is not None:
                    group["sum"] += value
                    group["valued"] += 1

        def sort_key(entry):
            (floor, zone, type_label, part), _ = entry
            return (-(floor if floor is not None else -10 ** 6), zone in (NO_ZONE, UNKNOWN_POSITION), zone.lower(),
                    type_label, part.lower())

        self.rows = []
        for (floor, zone, type_label, part), group in sorted(groups.items(), key=sort_key):
            self.rows.append({
                "story": stories.get(floor, "" if floor is None else str(floor)),
                "zone": zone,
                "type": type_label,
                "part": part,
                "count": len(group["elements"]),
                "sum": group["sum"] if parameter and group["valued"] else None,
                "valued": group["valued"],
                "elements": group["elements"],
            })
        self.sum_header = f"Sum of {parameter}" if parameter else ""
        self._show(parameter, values)

        if parameter and items and not any(v is not None for v in values.values()):
            notes.append(f"No counted element has a numeric parameter {parameter}.")
        text = f"Counted {len(items)} element(s) in {len(self.rows)} row(s)."
        self.window.set_status(" ".join([text] + notes), "warning" if notes else "ok")

    def _read_gdl_elements(self, scope: str, element_types: list[str]) -> list[dict]:
        elements = get_elements(scope, element_types)
        details = get_details_of_elements(elements, ["type", "floorIndex", "details"])
        labels = {key: label for key, _, label in TYPES}
        items, openings = [], []
        for element, detail in zip(elements, details):
            if "error" in detail or detail.get("type") not in element_types:
                continue
            specific = detail.get("details") or {}
            item = {
                "elementId": element["elementId"],
                "type": labels[detail["type"]],
                "part": (specific.get("libPart") or {}).get("name") or "(unknown library part)",
                "floorIndex": detail.get("floorIndex"),
                "points": [],
            }
            origin = specific.get("origin")
            if isinstance(origin, dict) and "x" in origin:
                item["points"] = [(origin["x"], origin["y"])]
            elif detail["type"] in ("Door", "Window"):
                openings.append((item, specific))
            items.append(item)
        if openings and self.by_zone.get():
            self._place_openings(openings)
        return items

    def _place_openings(self, openings: list[tuple[dict, dict]]) -> None:
        """ A door or window has no position of its own: it is centerOffset along its wall's reference
        line. Points on both sides of the wall are tried, so it is found in the zone next to the wall. """
        wall_guids = list(dict.fromkeys(s["ownerElementId"]["guid"] for _, s in openings
                                        if isinstance(s.get("ownerElementId"), dict)))
        walls = {}
        details = get_details_of_elements([{"elementId": {"guid": g}} for g in wall_guids], ["type", "details"])
        for guid, detail in zip(wall_guids, details):
            specific = detail.get("details") or {}
            if detail.get("type") == "Wall" and "begCoordinate" in specific and "endCoordinate" in specific:
                walls[guid] = specific
        for item, specific in openings:
            wall = walls.get((specific.get("ownerElementId") or {}).get("guid"))
            offset = _number(specific.get("centerOffset"))
            if wall is None or offset is None:
                continue
            bx, by = wall["begCoordinate"]["x"], wall["begCoordinate"]["y"]
            ex, ey = wall["endCoordinate"]["x"], wall["endCoordinate"]["y"]
            length = math.hypot(ex - bx, ey - by)
            if length < 1e-9:
                continue
            ux, uy = (ex - bx) / length, (ey - by) / length
            px, py = bx + ux * offset, by + uy * offset
            # the reference line may be on either face, so step a full thickness off it
            thickness = max(_number(wall.get("begThickness")) or 0.0, _number(wall.get("endThickness")) or 0.0)
            side = (thickness or 0.3) + 0.1
            item["points"] = [(px, py), (px - uy * side, py + ux * side), (px + uy * side, py - ux * side)]

    def _read_electrical_devices(self, scope: str, current_story, levels: list) -> tuple[list[dict], str]:
        try:
            listed = run_tapir("GetMEPElements", {"elementTypes": ["ElectricalDevice"]}).get("elements", [])
        except ArchicadError as e:
            print(f"Electrical devices are not read: {e}")
            return [], "Electrical devices need Archicad 30."
        elements = [{"elementId": e["elementId"]} for e in listed if e.get("type") == "ElectricalDevice"]
        if scope == "selection":
            selected = {e["elementId"]["guid"] for e in run_tapir("GetSelectedElements").get("elements", [])}
            elements = [e for e in elements if e["elementId"]["guid"] in selected]
        if not elements:
            return [], ""

        devices = []
        for chunk in batched(elements):
            devices += run_tapir("GetElectricalDevices", {"elements": chunk}).get("electricalDevices", [])
        headers = get_details_of_elements(elements, ["floorIndex"])
        names = self._library_part_names([d.get("libraryPartId") for d in devices if "error" not in d])

        items = []
        for element, device, header in zip(elements, devices, headers):
            if "error" in device:
                continue
            anchor = device.get("anchorPoint") or {}
            floor = header.get("floorIndex") if "error" not in header else None
            if floor is None and "z" in anchor:
                # the highest story at or below the device
                floor = next((index for level, index in reversed(levels) if level <= anchor["z"] + 1e-6),
                             levels[0][1] if levels else None)
            if scope == "story" and floor != current_story:
                continue
            part = names.get((device.get("libraryPartId") or {}).get("guid", "").upper())
            if not part:
                load = device.get("loadClassificationName") or ""
                part = f"Electrical device ({load})" if load else "(unknown library part)"
            items.append({
                "elementId": element["elementId"],
                "type": "Electrical device",
                "part": part,
                "floorIndex": floor,
                "points": [(anchor["x"], anchor["y"])] if "x" in anchor and "y" in anchor else [],
            })
        return items, ""

    def _library_part_names(self, part_ids: list[dict | None]) -> dict[str, str]:
        """ The names of library parts given by their main GUID. Archicad finds a library part
        by the main GUID of its unique ID alone, so the revision GUID is left zero. """
        guids = list(dict.fromkeys((p or {}).get("guid", "").upper() for p in part_ids if p))
        guids = [g for g in guids if g.strip("0-")]
        if not guids:
            return {}
        try:
            parts = run_tapir("GetLibraryPartTypeDetails", {
                "libraryParts": [{"unId": f"{{{g}}}-{NULL_REVISION}"} for g in guids]}).get("libraryParts", [])
        except ArchicadError as e:
            print(f"Cannot read the names of the electrical devices' library parts: {e}")
            return {}
        return {guid: part["name"] for guid, part in zip(guids, parts) if "error" not in part and part.get("name")}

    def _read_zones(self) -> dict[int, list[Zone]]:
        elements = run_tapir("GetElementsByType", {"elementType": "Zone"}).get("elements", [])
        details = get_details_of_elements(elements, ["floorIndex", "details"])
        zones: dict[int, list[Zone]] = {}
        for detail in details:
            if "error" in detail:
                continue
            specific = detail.get("details") or {}
            label = f"{specific.get('numberStr', '')} {specific.get('name', '')}".strip() or "(unnamed zone)"
            zone = Zone(label, specific.get("polygonOutline"), specific.get("holes"))
            if len(zone.outline) >= 3:
                zones.setdefault(detail.get("floorIndex"), []).append(zone)
        return zones

    @staticmethod
    def _zone_of(item: dict, zones: list[Zone]) -> str:
        if not item["points"]:
            return UNKNOWN_POSITION
        # the smallest zone wins: a room drawn inside a larger zone, a room rather than the corridor
        found = [zone for zone in zones if any(zone.contains(x, y) for x, y in item["points"])]
        return min(found, key=lambda z: z.area).label if found else NO_ZONE

    def _read_parameter(self, items: list[dict], parameter: str) -> dict[str, float | None]:
        wanted = parameter.lower()
        values: dict[str, float | None] = {}
        for chunk in batched(items, GDL_BATCH_SIZE):
            result = run_tapir("GetGDLParametersOfElements", {
                "elements": [{"elementId": i["elementId"]} for i in chunk]}).get("gdlParametersOfElements", [])
            for item, entry in zip(chunk, result):
                if "error" in entry:
                    continue
                for p in entry.get("parameters", []):
                    if p.get("name", "").lower() == wanted or (p.get("displayName") or "").lower() == wanted:
                        values[item["elementId"]["guid"]] = _number(p.get("value"))
                        break
        return values

    # ---- showing ----

    def _show(self, parameter: str, values: dict) -> None:
        self.tree.delete(*self.tree.get_children())
        self.row_ids = {}
        self.tree.heading("sum", text=self.sum_header or "Sum")
        for index, row in enumerate(self.rows):
            row_id = self.tree.insert("", "end", values=(
                row["story"], row["zone"], row["type"], row["part"], row["count"], self._format(row["sum"])))
            self.row_ids[row_id] = index
        total = sum(r["count"] for r in self.rows)
        self.stat_labels["elements"].configure(text=str(total))
        self.stat_labels["parts"].configure(text=str(len({(r["type"], r["part"]) for r in self.rows})))
        zones = {(r["story"], r["zone"]) for r in self.rows if r["zone"] and r["zone"] not in (NO_ZONE, UNKNOWN_POSITION)}
        self.stat_labels["zones"].configure(text=str(len(zones)) if self.by_zone.get() else "–")
        if parameter:
            numbers = [v for v in values.values() if v is not None]
            self.stat_labels["sum"].configure(text=self._format(sum(numbers)) if numbers else "–")
            self.sum_caption.configure(text=f"sum of {parameter}")
        else:
            self.stat_labels["sum"].configure(text="–")
            self.sum_caption.configure(text="sum")

    @staticmethod
    def _format(value: float | None) -> str:
        if value is None:
            return ""
        return str(int(round(value))) if abs(value - round(value)) < 1e-9 else f"{value:.3f}".rstrip("0")

    # ---- actions ----

    def _chosen_elements(self, rows: list[str]) -> list[dict]:
        guids = dict.fromkeys(e["elementId"]["guid"] for r in rows if r in self.row_ids
                              for e in self.rows[self.row_ids[r]]["elements"])
        return [{"elementId": {"guid": guid}} for guid in guids]

    def select_chosen(self) -> None:
        elements = self._chosen_elements(list(self.tree.selection()) or list(self.row_ids))
        if not elements:
            self.window.set_status("There are no elements to select. Press Count first.", "warning")
            return
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        elements = self._chosen_elements([self.tree.focus()])
        if elements:
            select_elements(elements)
            self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def export(self) -> None:
        if not self.rows:
            self.window.set_status("Press Count first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the counts",
                                            initialfile=f"Object count {stamp}.csv", defaultextension=".csv",
                                            filetypes=list(reversed(FILE_TYPES)))
        if not path:
            return
        headers = ["Story", "Zone", "Type", "Library part", "Count"]
        if self.sum_header:
            headers += [self.sum_header, "Elements with the parameter"]
        rows = []
        for r in self.rows:
            row = [r["story"], r["zone"], r["type"], r["part"], r["count"]]
            if self.sum_header:
                row += [round(r["sum"], 6) if r["sum"] is not None else "", r["valued"]]
            rows.append(row)
        write_table(path, headers, rows, "Object count")
        self.window.set_status(f"Saved {len(rows)} row(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=lambda: self.window.set_status("Choose what to count and press Count."))


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ObjectCounter().run()
