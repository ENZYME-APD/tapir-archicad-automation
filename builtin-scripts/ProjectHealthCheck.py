# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Project Health Check

Runs quality checks on the model and lists what it finds: elements without an
Element ID, zones with the same number, unclassified elements, duplicate
elements in the same place, very small elements, elements far from the project
origin, elements on hidden layers, stories without elements and layers that no
floor plan element uses. The findings can be highlighted and selected in
Archicad, exported to Excel or CSV, and turned into Issues.
"""

import datetime
import math
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements,
                                run_command, run_tapir, select_elements)
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Project Health Check"
DIMMED_COLOR = [200, 200, 200, 160]
SEVERITIES = {
    # marker, highlight color, text color
    "error": ("✖", [220, 30, 30, 255], "#c62828"),
    "warning": ("⚠", [245, 140, 0, 255], "#a15c00"),
    "info": ("ℹ", [30, 110, 220, 255], "#1f5fa8"),
}
OK_MARK = "✔"
# the 3D model elements the element checks look at
MODEL_TYPES = {
    "Wall", "Column", "Beam", "Window", "Door", "Object", "Lamp", "Slab", "Roof", "Mesh", "Zone", "CurtainWall",
    "Shell", "Skylight", "Morph", "Stair", "Railing", "Opening",
}
ARCHICAD_LAYER_INDEX = 1  # the built-in Archicad Layer, which cannot be deleted
SHOWN_PER_CHECK = 2000  # longer lists are shortened in the window, not in the export or the Issues
CHECKS = [
    # key, title, severity, on by default
    ("missing_id", "Elements without an Element ID", "warning", True),
    ("zone_numbers", "Zones with the same number", "warning", True),
    ("unclassified", "Unclassified elements", "warning", True),
    ("duplicates", "Duplicate elements in the same place", "error", True),
    ("tiny", "Very small elements (under 1 cm)", "warning", True),
    ("far", "Elements far from the project origin", "warning", True),
    ("hidden_layers", "Elements on hidden layers", "info", False),
    ("empty_stories", "Stories without elements", "info", True),
    ("unused_layers", "Layers no floor plan element uses", "info", False),
]


def format_distance(meters: float) -> str:
    return f"{meters / 1000:.2f} km" if meters >= 1000 else f"{meters:.1f} m"


class ProjectHealthCheck:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Checks the model for common problems. Choose the checks and press Run checks. "
            "Select findings to highlight them; double-click one to select it in Archicad.",
            min_width=760, min_height=640)
        self.enabled = {key: tk.BooleanVar(value=default) for key, _, _, default in CHECKS}
        self.classification_systems: list[dict] = []
        # key -> list of findings: {"label", "type", "story", "layer", "note", "elementId" (optional)}
        self.results: dict[str, list[dict]] = {}
        # tree row id -> (check key, finding index or None for the check's own row)
        self.rows: dict[str, tuple[str, int | None]] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_chosen)
        self.window.add_button("Create Issues", self.create_issues)
        self.window.add_button("Export...", self.export)
        self.window.add_button("Run checks", self.run_checks, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Checks", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        for index, (key, title, severity, _) in enumerate(CHECKS):
            row, column = index // 2, (index % 2) * 2
            ttk.Checkbutton(options, text=title, variable=self.enabled[key]).grid(
                row=row, column=column, sticky="w", pady=1, padx=(0 if column == 0 else 18, 0))
        extra = ttk.Frame(options)
        extra.grid(row=len(CHECKS) // 2 + 1, column=0, columnspan=4, sticky="w", pady=(6, 0))
        ttk.Label(extra, text="Classification system:").grid(row=0, column=0, sticky="w")
        self.classification_system = tk.StringVar()
        self.system_box = ttk.Combobox(extra, textvariable=self.classification_system, width=26, state="readonly")
        self.system_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(extra, text="Far means more than:").grid(row=0, column=2, sticky="w", padx=(18, 0))
        self.far_distance = tk.StringVar(value="1000")
        ttk.Spinbox(extra, textvariable=self.far_distance, from_=10, to=100000, increment=100, width=8).grid(
            row=0, column=3, sticky="w", padx=(6, 0))
        ttk.Label(extra, text="m", style="Muted.TLabel").grid(row=0, column=4, sticky="w", padx=(4, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("elements", "elements checked"), ("error", "errors"),
                                                 ("warning", "warnings"), ("info", "notes")]):
            summary.columnconfigure(column, weight=1)
            value = ttk.Label(summary, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(summary, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        self.tree = make_tree(body, [
            ("type", "Type", 110, "w"),
            ("story", "Story", 120, "w"),
            ("layer", "Layer", 140, "w"),
            ("note", "Note", 200, "w"),
        ], height=14, show_tree=True)
        self.tree.heading("#0", text="Check / Element ID", anchor="w")
        self.tree.column("#0", width=280, stretch=True)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        for severity, (_, _, color) in SEVERITIES.items():
            self.tree.tag_configure(severity, foreground=color)
        self.tree.tag_configure("ok", foreground="#1b7f3b")
        self.tree.tag_configure("more", foreground="#777777")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._highlight)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    def load_classification_systems(self) -> None:
        try:
            systems = run_command("API.GetAllClassificationSystems").get("classificationSystems", [])
        except ArchicadError as e:
            systems = []
            print(f"Cannot read the classification systems: {e}")
        self.classification_systems = systems
        names = [f"{s.get('name', '')} {s.get('version', '')}".strip() for s in systems]
        self.system_box.configure(values=names)
        if names:
            self.classification_system.set(names[0])
        else:
            self.enabled["unclassified"].set(False)
        self.window.set_status("Choose the checks and press Run checks.")

    # ---- checking ----

    def run_checks(self) -> None:
        try:
            far_limit = max(1.0, float(self.far_distance.get().replace(",", ".")))
        except ValueError:
            self.window.set_status("The distance must be a number of meters.", "error")
            return
        on = {key for key, var in self.enabled.items() if var.get()}
        if not on:
            self.window.set_status("Choose at least one check.", "warning")
            return

        with self.window.busy("Reading the elements..."):
            elements = get_elements("project")
            self.window.set_status(f"Reading the details of {len(elements)} elements...")
            details = get_details_of_elements(elements, ["type", "id", "floorIndex", "layerIndex", "hotlinkId"])
            story_list = run_tapir("GetStories").get("stories", [])
            stories = {s["index"]: s.get("name") or f"Story {s['index']}" for s in story_list}
            layer_headers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
            layer_names = {int(a["index"]): a["name"] for a in layer_headers}

            items = []
            hotlinked = 0
            for element, detail in zip(elements, details):
                if "error" in detail:
                    continue
                if "hotlinkId" in detail:
                    hotlinked += 1
                    continue
                items.append({
                    "elementId": element["elementId"],
                    "type": detail.get("type", ""),
                    "id": detail.get("id", ""),
                    "floorIndex": detail.get("floorIndex"),
                    "layerIndex": int(detail.get("layerIndex", -1)),
                })
            model = [i for i in items if i["type"] in MODEL_TYPES]

            self.results = {}
            self.layer_names, self.stories = layer_names, stories
            if "missing_id" in on:
                self.results["missing_id"] = [self._finding(i) for i in model if not i["id"].strip()]
            if "zone_numbers" in on:
                self.window.set_status("Checking the zone numbers...")
                self.results["zone_numbers"] = self._check_zone_numbers([i for i in model if i["type"] == "Zone"])
            if "unclassified" in on:
                self.window.set_status("Checking the classifications...")
                self.results["unclassified"] = self._check_classifications(model)
            if on & {"duplicates", "tiny", "far"}:
                self.window.set_status("Measuring the elements...")
                self._check_geometry(model, on, far_limit)
            if "hidden_layers" in on:
                self.window.set_status("Checking the layers...")
                self.results["hidden_layers"] = self._check_hidden_layers(items, layer_headers)
            if "empty_stories" in on:
                used = {i["floorIndex"] for i in items}
                levels = {s["index"]: s.get("level", 0.0) for s in story_list}
                self.results["empty_stories"] = [
                    {"label": name, "type": "Story", "story": name, "layer": "", "note": f"level {levels.get(index, 0.0):.2f} m"}
                    for index, name in sorted(stories.items()) if index not in used]
            if "unused_layers" in on:
                used = {i["layerIndex"] for i in items}
                self.results["unused_layers"] = [
                    {"label": name, "type": "Layer", "story": "", "layer": name,
                     "note": "sections, details, worksheets and layouts are not checked"}
                    for index, name in sorted(layer_names.items(), key=lambda kv: kv[1].lower())
                    if index not in used and index != ARCHICAD_LAYER_INDEX]

        self._show(len(items))
        found = sum(len(f) for f in self.results.values())
        skipped = f" {hotlinked} hotlinked element(s) were skipped." if hotlinked else ""
        if found:
            self.window.set_status(f"Done: {found} finding(s).{skipped}", "warning")
        else:
            self.window.set_status(f"Done: no problems found.{skipped}", "ok")

    def _finding(self, item: dict, note: str = "") -> dict:
        return {
            "label": item["id"] or "(no ID)",
            "type": item["type"],
            "story": self.stories.get(item["floorIndex"], str(item["floorIndex"])),
            "layer": self.layer_names.get(item["layerIndex"], ""),
            "note": note,
            "elementId": item["elementId"],
        }

    def _check_zone_numbers(self, zones: list[dict]) -> list[dict]:
        details = get_details_of_elements([{"elementId": z["elementId"]} for z in zones], ["details"])
        by_number: dict[str, list[dict]] = {}
        for zone, detail in zip(zones, details):
            number = (detail.get("details") or {}).get("numberStr", "").strip()
            if number:
                by_number.setdefault(number, []).append(zone)
        return [self._finding(zone, f"number {number}, {len(same)} zones")
                for number, same in sorted(by_number.items()) if len(same) > 1 for zone in same]

    def _check_classifications(self, model: list[dict]) -> list[dict]:
        names = list(self.system_box.cget("values"))
        if self.classification_system.get() not in names:
            return []
        system = self.classification_systems[names.index(self.classification_system.get())]
        system_ids = [{"classificationSystemId": system["classificationSystemId"]}]
        findings = []
        for chunk in batched(model):
            result = run_tapir("GetClassificationsOfElements", {
                "elements": [{"elementId": i["elementId"]} for i in chunk],
                "classificationSystemIds": system_ids,
            }).get("elementClassifications", [])
            for item, entry in zip(chunk, result):
                # an element without classificationItemId is Unclassified; errors are not findings
                classification_id = (entry.get("classificationIds") or [{}])[0].get("classificationId")
                if classification_id is not None and "classificationItemId" not in classification_id:
                    findings.append(self._finding(item, f"in {self.classification_system.get()}"))
        return findings

    def _check_geometry(self, model: list[dict], on: set[str], far_limit: float) -> None:
        boxes = []
        for chunk in batched(model):
            result = run_tapir("Get3DBoundingBoxes", {"elements": [{"elementId": i["elementId"]} for i in chunk]})
            boxes += [b.get("boundingBox3D") for b in result.get("boundingBoxes3D", [])]
        measured = [(item, box) for item, box in zip(model, boxes) if box]

        if "duplicates" in on:
            same_place: dict[tuple, list[dict]] = {}
            for item, box in measured:
                key = (item["type"], item["floorIndex"]) + tuple(round(box[k], 3) for k in
                                                                 ("xMin", "yMin", "zMin", "xMax", "yMax", "zMax"))
                same_place.setdefault(key, []).append(item)
            groups = [group for group in same_place.values() if len(group) > 1]
            self.results["duplicates"] = [
                self._finding(item, f"group {number}: {len(group)} {item['type']} elements")
                for number, group in enumerate(groups, start=1) for item in group]
        if "tiny" in on:
            self.results["tiny"] = [
                self._finding(item, f"{max(b['xMax'] - b['xMin'], b['yMax'] - b['yMin'], b['zMax'] - b['zMin']) * 1000:.1f} mm")
                for item, b in measured
                if max(b["xMax"] - b["xMin"], b["yMax"] - b["yMin"], b["zMax"] - b["zMin"]) < 0.01]
        if "far" in on:
            findings = []
            for item, b in measured:
                distance = max(math.hypot(x, y) for x in (b["xMin"], b["xMax"]) for y in (b["yMin"], b["yMax"]))
                distance = max(distance, abs(b["zMin"]), abs(b["zMax"]))
                if distance > far_limit:
                    findings.append((distance, self._finding(item, f"{format_distance(distance)} away")))
            self.results["far"] = [f for _, f in sorted(findings, key=lambda f: -f[0])]

    def _check_hidden_layers(self, items: list[dict], layer_headers: list[dict]) -> list[dict]:
        attribute_ids = [{"attributeId": a["attributeId"]} for a in layer_headers]
        try:
            layers = run_tapir("GetLayers", {"attributeIds": attribute_ids, "fields": ["isHidden"]}).get("layers", [])
        except ArchicadError:
            # Add-Ons that do not know the fields parameter
            layers = run_tapir("GetLayers", {"attributeIds": attribute_ids}).get("layers", [])
        hidden = {int(l["index"]) for l in layers if l.get("isHidden")}
        return [self._finding(i) for i in items if i["layerIndex"] in hidden]

    # ---- showing ----

    def _show(self, element_count: int) -> None:
        self.tree.delete(*self.tree.get_children())
        self.rows = {}
        counts = {"error": 0, "warning": 0, "info": 0}
        for key, title, severity, _ in CHECKS:
            if key not in self.results:
                continue
            findings = self.results[key]
            counts[severity] += len(findings)
            mark = SEVERITIES[severity][0] if findings else OK_MARK
            check_row = f"check:{key}"
            self.tree.insert("", "end", iid=check_row, text=f"{mark}  {title}", open=False,
                             values=("", "", "", f"{len(findings)} found" if findings else "none"),
                             tags=(severity if findings else "ok",))
            self.rows[check_row] = (key, None)
            for index, finding in enumerate(findings[:SHOWN_PER_CHECK]):
                row = f"{key}:{index}"
                self.tree.insert(check_row, "end", iid=row, text=finding["label"],
                                 values=(finding["type"], finding["story"], finding["layer"], finding["note"]))
                self.rows[row] = (key, index)
            if len(findings) > SHOWN_PER_CHECK:
                self.tree.insert(check_row, "end", text=f"... and {len(findings) - SHOWN_PER_CHECK} more (see the export)",
                                 tags=("more",))
        self.stat_labels["elements"].configure(text=str(element_count))
        for severity, count in counts.items():
            color = SEVERITIES[severity][2] if count else "#1b7f3b"
            self.stat_labels[severity].configure(text=str(count), foreground=color)
        self._highlight()

    def _chosen(self) -> list[tuple[str, dict]]:
        """ The chosen findings with their check: the selected rows, or all when none is selected. """
        rows = [r for r in self.tree.selection() if r in self.rows] or [r for r in self.rows if self.rows[r][1] is None]
        chosen, seen = [], set()
        for row in rows:
            key, index = self.rows[row]
            for i, finding in ([(index, self.results[key][index])] if index is not None else enumerate(self.results[key])):
                if (key, i) not in seen:
                    seen.add((key, i))
                    chosen.append((key, finding))
        return chosen

    def _highlight(self) -> None:
        severity_of = {key: severity for key, _, severity, _ in CHECKS}
        colors: dict[str, list[int]] = {}
        for key, finding in self._chosen():
            if "elementId" in finding:
                colors.setdefault(finding["elementId"]["guid"], SEVERITIES[severity_of[key]][1])
        if not colors:
            self.clear_highlight()
            return
        run_tapir("HighlightElements", {
            "elements": [{"elementId": {"guid": guid}} for guid in colors],
            "highlightedColors": list(colors.values()),
            "wireframe3D": False,
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    # ---- actions ----

    def _chosen_elements(self) -> list[dict]:
        guids = dict.fromkeys(f["elementId"]["guid"] for _, f in self._chosen() if "elementId" in f)
        return [{"elementId": {"guid": guid}} for guid in guids]

    def select_chosen(self) -> None:
        elements = self._chosen_elements()
        if not elements:
            self.window.set_status("There are no elements to select.", "warning")
            return
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row in self.rows and self.rows[row][1] is not None:
            key, index = self.rows[row]
            finding = self.results[key][index]
            if "elementId" in finding:
                select_elements([{"elementId": finding["elementId"]}])
                self.window.set_status(f"Selected {finding['label']} in Archicad.", "ok")

    def export(self) -> None:
        if not self.results:
            self.window.set_status("Run the checks first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the findings",
                                            initialfile=f"Health check {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        titles = {key: (title, severity) for key, title, severity, _ in CHECKS}
        rows = [[titles[key][0], titles[key][1], f["label"], f["type"], f["story"], f["layer"], f["note"],
                 f["elementId"]["guid"] if "elementId" in f else ""]
                for key, findings in self.results.items() for f in findings]
        write_table(path, ["Check", "Severity", "Name", "Type", "Story", "Layer", "Note", "GUID"], rows, "Health check")
        self.window.set_status(f"Exported {len(rows)} finding(s) to {os.path.basename(path)}.", "ok")

    def create_issues(self) -> None:
        by_check: dict[str, list[dict]] = {}
        for key, finding in self._chosen():
            if "elementId" in finding:
                by_check.setdefault(key, []).append({"elementId": finding["elementId"]})
        if not by_check:
            self.window.set_status("There are no findings with elements to make Issues of.", "warning")
            return
        titles = {key: title for key, title, _, _ in CHECKS}
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Create {len(by_check)} Issue(s) in the Issue Organizer, "
                                                      "one per check, with the found elements attached?"):
            return
        with self.window.busy("Creating the Issues..."):
            for key, elements in by_check.items():
                issue = run_tapir("CreateIssue", {"name": f"{titles[key]} ({len(elements)})", "tagText": SCRIPT_NAME})
                run_tapir("AttachElementsToIssue", {"issueId": issue["issueId"], "elements": elements, "type": "Highlight"})
        self.window.set_status(f"Created {len(by_check)} Issue(s).", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_classification_systems, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ProjectHealthCheck().run()
