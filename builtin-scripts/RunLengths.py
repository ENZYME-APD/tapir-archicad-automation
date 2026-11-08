# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "quantities-scheduling"
# changes = "files"
# ///

"""
Run Lengths

Totals the lengths of pipe, duct and wiring runs in two ways, side by side:
the MEP Modeler routes and flexible segments per domain, MEP system and size,
and the 2D lines, polylines and arcs drawn for them, per layer, pen or line
type. In Archicad 30 each electrical circuit's estimated cable length is
listed as well. The table can be copied or saved as CSV or Excel. It reads
only; nothing in the project is changed.
"""

import math
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Run Lengths"
ALL_STORIES = "All stories"
DRAWING_TYPES = ["Line", "PolyLine", "Arc", "Circle"]
GROUP_BY = ["Layer", "Pen", "Line type", "Layer, pen and line type"]
ROUTE_BATCH = 200
EXPORT_HEADERS = ["Source", "Domain", "System or group", "Size", "Count", "Length (m)"]


def distance(a: dict, b: dict) -> float:
    return math.sqrt((a["x"] - b["x"]) ** 2 + (a["y"] - b["y"]) ** 2 + (a.get("z", 0.0) - b.get("z", 0.0)) ** 2)


def path_length(points: list[dict]) -> float:
    return sum(distance(p, q) for p, q in zip(points, points[1:]))


def polyline_length(coordinates: list[dict], arcs: list[dict]) -> float:
    """ A 2D polyline, its arc edges measured along the arc (arcAngle on the edge begIndex -> endIndex). """
    arc_angles = {(a["begIndex"], a["endIndex"]): a["arcAngle"] for a in arcs or []}
    total = 0.0
    for index, (p, q) in enumerate(zip(coordinates, coordinates[1:])):
        chord = distance(p, q)
        angle = abs(arc_angles.get((index, index + 1), 0.0))
        if angle > 1e-9 and chord > 0.0:
            total += chord * angle / (2.0 * math.sin(angle / 2.0))
        else:
            total += chord
    return total


def ellipse_length(radius: float, ratio: float, begin: float, end: float) -> float:
    """ The length of an arc of a circle (ratio 1) or, numerically, of an ellipse. """
    sweep = (end - begin) % (2.0 * math.pi)
    if sweep < 1e-9:
        sweep = 2.0 * math.pi
    if abs(ratio - 1.0) < 1e-9:
        return radius * sweep
    steps = 360
    step = sweep / steps
    total = 0.0
    for i in range(steps):
        t = begin + (i + 0.5) * step
        total += math.hypot(radius * math.sin(t), radius * ratio * math.cos(t)) * step
    return total


def drawing_length(element_type: str, details: dict) -> float:
    if element_type == "Line":
        return distance(details["begCoordinate"], details["endCoordinate"])
    if element_type == "PolyLine":
        return polyline_length(details.get("coordinates", []), details.get("arcs", []))
    if element_type == "Arc":
        return ellipse_length(details["radius"], details.get("ratio", 1.0), details["begAngle"], details["endAngle"])
    if element_type == "Circle":
        return ellipse_length(details["radius"], details.get("ratio", 1.0), 0.0, 2.0 * math.pi)
    return 0.0


def size_label(segment: dict) -> str:
    """ The cross section in millimetres, as Archicad shows it. """
    shape = segment.get("crossSectionShape", "")
    width = segment.get("crossSectionWidth", 0.0) * 1000.0
    height = segment.get("crossSectionHeight", 0.0) * 1000.0
    if shape == "Circular":
        return f"Ø{width:g}"
    return f"{width:g}×{height:g}" + ("" if shape in ("Rectangular", "") else f" {shape}")


def guid_of(item: dict | None) -> str | None:
    if not item:
        return None
    return (item.get("elementId") or item).get("guid")


class RunLengths:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Totals the lengths of the MEP routes per domain, system and size, and of the 2D lines, polylines "
            "and arcs per layer, pen or line type, side by side. MEP route lengths are measured along the "
            "centre line from node to node, elbows included. In Archicad 30 the electrical circuits' estimated "
            "cable lengths are listed as well. Nothing in the project is changed.",
            min_width=980, min_height=600)
        self.mep_rows: list[list] = []
        self.drawing_rows: list[list] = []
        self.story_indices: dict[str, int | None] = {ALL_STORIES: None}
        self._build_body()
        self.window.add_button("Copy to clipboard", self.copy_to_clipboard)
        self.window.add_button("Save as...", self.save_as)
        self.window.add_button("Calculate", self.calculate, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        ttk.Label(options, text="Story:").grid(row=0, column=0, sticky="w")
        self.story = tk.StringVar(value=ALL_STORIES)
        self.story_box = ttk.Combobox(options, textvariable=self.story, values=[ALL_STORIES], state="readonly", width=24)
        self.story_box.grid(row=0, column=1, sticky="w", padx=(6, 16))
        self.only_selection = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Only the selected elements", variable=self.only_selection).grid(
            row=0, column=2, sticky="w")
        self.by_network = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Split MEP runs by connected network", variable=self.by_network).grid(
            row=0, column=3, sticky="w", padx=(16, 0))

        ttk.Label(options, text="Group 2D lines by:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.group_by = tk.StringVar(value=GROUP_BY[0])
        ttk.Combobox(options, textvariable=self.group_by, values=GROUP_BY, state="readonly", width=24).grid(
            row=1, column=1, sticky="w", padx=(6, 16), pady=(6, 0))
        ttk.Label(options, text="Only groups containing:").grid(row=1, column=2, sticky="w", pady=(6, 0))
        self.group_filter = tk.StringVar(value="")
        ttk.Entry(options, textvariable=self.group_filter, width=24).grid(
            row=1, column=3, sticky="w", padx=(6, 0), pady=(6, 0))

        stats = ttk.Frame(body, padding=(0, 10, 0, 6))
        stats.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("mep", "m of MEP routes"), ("circuits", "m of estimated cable"),
                                                 ("drawing", "m of 2D lines")]):
            stats.columnconfigure(column, weight=1)
            value = ttk.Label(stats, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(stats, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        panes = ttk.PanedWindow(body, orient="horizontal")
        panes.grid(row=2, column=0, sticky="nsew")
        left = ttk.LabelFrame(panes, text="MEP Modeler", padding=(6, 4))
        left.columnconfigure(0, weight=1)
        left.rowconfigure(0, weight=1)
        self.mep_tree = make_tree(left, [
            ("system", "System", 160, "w"),
            ("source", "Kind", 110, "w"),
            ("domain", "Domain", 90, "w"),
            ("size", "Size (mm)", 90, "w"),
            ("count", "Count", 60, "e"),
            ("length", "Length (m)", 90, "e"),
        ], height=14)
        self.mep_tree.container.grid(row=0, column=0, sticky="nsew")
        right = ttk.LabelFrame(panes, text="2D lines", padding=(6, 4))
        right.columnconfigure(0, weight=1)
        right.rowconfigure(0, weight=1)
        self.drawing_tree = make_tree(right, [
            ("group", "Group", 220, "w"),
            ("count", "Elements", 70, "e"),
            ("length", "Length (m)", 90, "e"),
        ], height=14)
        self.drawing_tree.container.grid(row=0, column=0, sticky="nsew")
        panes.add(left, weight=3)
        panes.add(right, weight=2)

    # ---- reading ----

    def _load_stories(self) -> None:
        stories = run_tapir("GetStories").get("stories", [])
        self.story_indices = {ALL_STORIES: None}
        for story in sorted(stories, key=lambda s: s["index"], reverse=True):
            name = story.get("name") or f"Story {story['index']}"
            self.story_indices[f"{story['index']}. {name}"] = story["index"]
        self.story_box.configure(values=list(self.story_indices))
        if self.story.get() not in self.story_indices:
            self.story.set(ALL_STORIES)

    def calculate(self) -> None:
        notes: list[str] = []
        with self.window.busy("Reading the stories..."):
            self._load_stories()
            floor = self.story_indices.get(self.story.get())
            selection = None
            if self.only_selection.get():
                selection = {guid_of(e) for e in run_tapir("GetSelectedElements").get("elements", [])}
            self.window.set_status("Reading the MEP routes...")
            mep_rows, mep_total, circuit_total = self._mep_lengths(floor, selection, notes)
            self.window.set_status("Reading the 2D lines...")
            drawing_rows, drawing_total = self._drawing_lengths(floor, selection, notes)

        self.mep_rows, self.drawing_rows = mep_rows, drawing_rows
        self.stat_labels["mep"].configure(text=f"{mep_total:.2f}")
        self.stat_labels["circuits"].configure(text=f"{circuit_total:.2f}" if circuit_total is not None else "–")
        self.stat_labels["drawing"].configure(text=f"{drawing_total:.2f}")
        self._show()
        for note in notes:
            print(note)
        if notes:
            self.window.set_status(" ".join(notes[:2]) + (" See the palette output." if len(notes) > 2 else ""), "warning")
        elif not mep_rows and not drawing_rows:
            self.window.set_status("Nothing to measure here.", "info")
        else:
            self.window.set_status(f"{len(mep_rows)} MEP and {len(drawing_rows)} 2D line group(s) measured.", "ok")

    def _mep_lengths(self, floor: int | None, selection: set | None, notes: list[str]):
        totals: dict[tuple, list] = {}
        mep_total = 0.0
        try:
            elements = run_tapir("GetMEPElements", {"elementTypes": ["RoutingElement", "FlexibleSegment"]}).get("elements", [])
        except ArchicadError as e:
            notes.append(f"MEP routes not read ({e}); they need Archicad 28 or later.")
            elements = []
        routes = [{"elementId": e["elementId"]} for e in elements if e.get("type") == "RoutingElement"]
        flexibles = [{"elementId": e["elementId"]} for e in elements if e.get("type") == "FlexibleSegment"]

        system_names = {}
        if elements:
            try:
                system_names = {a["attributeId"]["guid"]: a["name"] for a in
                                run_tapir("GetAttributesByType", {"attributeType": "MEPSystem"}).get("attributes", [])}
            except ArchicadError as e:
                notes.append(f"MEP system names not read: {e}")

        networks: dict[str, str] = {}
        if elements and self.by_network.get():
            try:
                systems = run_tapir("GetMEPDistributionSystems").get("distributionSystems", [])
                for number, system in enumerate(systems, start=1):
                    for member in system.get("elements", []):
                        networks.setdefault(guid_of(member), f"Network {number}")
            except ArchicadError as e:
                notes.append(f"Connected networks not read: {e}")

        def add(key: tuple, length: float) -> None:
            entry = totals.setdefault(key, [0, 0.0])
            entry[0] += 1
            entry[1] += length

        failed = 0
        for chunk in batched(routes, ROUTE_BATCH):
            self.window.set_status(f"Reading the MEP routes... {len(totals)} group(s) so far")
            results = run_tapir("GetMEPRoutingElements", {"elements": chunk}).get("routingElements", [])
            for element, route in zip(chunk, results):
                if "error" in route:
                    failed += 1
                    continue
                if floor is not None and route.get("floorIndex") != floor:
                    continue
                route_guid = element["elementId"]["guid"]
                segments = route.get("segments", [])
                members = {route_guid}
                for segment in segments:
                    members.add(guid_of(segment))
                    members.update(guid_of(r) for r in segment.get("rigidSegments", []))
                if selection is not None and not members & selection:
                    continue
                network = next((networks[g] for g in members if g in networks), "(not connected)") if networks else ""
                system_id = guid_of(route.get("mepSystemId", {}).get("attributeId"))
                system = system_names.get(system_id, "(unknown system)")
                nodes = {guid_of(n): n["position"] for n in route.get("nodes", []) if "position" in n}
                polyline = route.get("polyline", [])
                for index, segment in enumerate(segments):
                    begin, end = nodes.get(guid_of(segment.get("beginNodeId"))), nodes.get(guid_of(segment.get("endNodeId")))
                    if begin is None or end is None:
                        if index + 1 >= len(polyline):
                            continue
                        begin, end = polyline[index], polyline[index + 1]
                    length = distance(begin, end)
                    mep_total += length
                    add(("Route", route.get("domain", ""), system, size_label(segment), network), length)

        for chunk in batched(flexibles, ROUTE_BATCH):
            results = run_tapir("GetMEPElementDetails", {"elements": chunk}).get("elementDetails", [])
            for element, details in zip(chunk, results):
                if "error" in details:
                    failed += 1
                    continue
                if selection is not None and element["elementId"]["guid"] not in selection:
                    continue
                points = details.get("controlPoints", [])
                if floor is not None or len(points) < 2:
                    # flexible segments have no home story in GetMEPElementDetails
                    continue
                length = path_length(points)
                mep_total += length
                network = networks.get(element["elementId"]["guid"], "(not connected)") if networks else ""
                add(("Flexible segment", details.get("domain", ""), "(no system)", "", network), length)
        if failed:
            notes.append(f"{failed} MEP element(s) could not be read.")
        if flexibles and floor is not None:
            notes.append("Flexible segments are only counted with All stories.")

        rows = [[source, domain, f"{system} – {network}" if network else system, size, count, round(length, 3)]
                for (source, domain, system, size, network), (count, length) in totals.items()]
        rows.sort(key=lambda r: (r[1], r[2], r[0], r[3]))

        circuit_total = None
        try:
            circuits = run_tapir("GetElectricalCircuits").get("electricalCircuits", [])
            circuit_total = 0.0
            circuit_rows = []
            for circuit in circuits:
                if "error" in circuit:
                    continue
                devices = {guid_of(d) for d in circuit.get("childDevices", []) + circuit.get("childPanels", [])}
                if selection is not None and guid_of(circuit) not in selection and not devices & selection \
                        and guid_of(circuit.get("ownerPanelId")) not in selection:
                    continue
                length = circuit.get("estimatedCableLength", 0.0)
                circuit_total += length
                circuit_rows.append(["Circuit", "Electrical", circuit.get("name", ""), circuit.get("wiringTypeName", ""),
                                     len(devices), round(length, 3)])
            rows += sorted(circuit_rows, key=lambda r: r[2])
        except ArchicadError as e:
            print(f"Electrical circuits not read ({e}); they need Archicad 30.")
        return rows, mep_total, circuit_total

    def _drawing_lengths(self, floor: int | None, selection: set | None, notes: list[str]):
        scope = "selection" if selection is not None else "project"
        elements = get_elements(scope, DRAWING_TYPES)
        if not elements:
            return [], 0.0
        layers = {a["index"]: a["name"] for a in run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])}
        line_types = {a["attributeId"]["guid"]: a["name"] for a in
                      run_tapir("GetAttributesByType", {"attributeType": "Line"}).get("attributes", [])}
        details = get_details_of_elements(elements, ["type", "floorIndex", "layerIndex", "details"])

        group_by = self.group_by.get()
        totals: dict[str, list] = {}
        total = 0.0
        failed = 0
        for detail in details:
            if "error" in detail or "details" not in detail:
                failed += 1
                continue
            if floor is not None and detail.get("floorIndex") != floor:
                continue
            specific = detail["details"]
            try:
                length = drawing_length(detail.get("type", ""), specific)
            except (KeyError, TypeError, ValueError):
                failed += 1
                continue
            layer = layers.get(int(detail.get("layerIndex", -1)), "(unknown layer)")
            pen = f"Pen {specific.get('linePenIndex', '?')}"
            line_type = line_types.get(guid_of(specific.get("lineTypeId")), "(unknown line type)")
            group = {"Layer": layer, "Pen": pen, "Line type": line_type}.get(group_by, f"{layer} · {pen} · {line_type}")
            entry = totals.setdefault(group, [0, 0.0])
            entry[0] += 1
            entry[1] += length
            total += length
        if failed:
            notes.append(f"{failed} 2D line(s) could not be measured.")
        rows = [["2D drawing", "", group, "", count, round(length, 3)] for group, (count, length) in sorted(totals.items())]
        return rows, total

    # ---- showing and saving ----

    def _filtered_drawing_rows(self) -> list[list]:
        text = self.group_filter.get().strip().lower()
        return [r for r in self.drawing_rows if text in r[2].lower()] if text else self.drawing_rows

    def _show(self) -> None:
        self.mep_tree.delete(*self.mep_tree.get_children())
        for source, domain, system, size, count, length in self.mep_rows:
            self.mep_tree.insert("", "end", values=(system, source, domain, size, count, f"{length:.3f}"))
        self.drawing_tree.delete(*self.drawing_tree.get_children())
        for _source, _domain, group, _size, count, length in self._filtered_drawing_rows():
            self.drawing_tree.insert("", "end", values=(group, count, f"{length:.3f}"))

    def _export_rows(self) -> list[list]:
        return self.mep_rows + self._filtered_drawing_rows()

    def copy_to_clipboard(self) -> None:
        rows = self._export_rows()
        if not rows:
            self.window.set_status("Press Calculate first.", "warning")
            return
        text = "\n".join("\t".join(str(v) for v in row) for row in [EXPORT_HEADERS] + rows)
        self.window.root.clipboard_clear()
        self.window.root.clipboard_append(text)
        self.window.set_status(f"Copied {len(rows)} row(s) to the clipboard.", "ok")

    def save_as(self) -> None:
        rows = self._export_rows()
        if not rows:
            self.window.set_status("Press Calculate first.", "warning")
            return
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the run lengths",
                                            defaultextension=".csv", initialfile="RunLengths.csv",
                                            filetypes=list(reversed(FILE_TYPES)))
        if not path:
            return
        write_table(path, EXPORT_HEADERS, rows, sheet_name="Run lengths")
        self.window.set_status(f"Saved {len(rows)} row(s) to {path}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.calculate)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    RunLengths().run()
