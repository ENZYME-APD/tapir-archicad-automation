# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Electrical and Plumbing Check

Before drawings are issued, lists the electrical and plumbing symbols that miss
data: GDL symbols whose circuit, board or number parameter (chosen by the user)
is empty or names a board that does not exist, Archicad 30 electrical devices
that are not connected to a circuit, Archicad 30 circuits without devices, and
sanitary objects that are outside every zone. Each group can be highlighted and
selected in Archicad. The script only reads the model; it changes nothing but
the highlight and the selection.
"""

import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, run_tapir,
                                select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Electrical and Plumbing Check"
DIMMED_COLOR = [200, 200, 200, 160]
ERROR_COLOR = [220, 30, 30, 255]
WARNING_COLOR = [245, 140, 0, 255]
SYMBOL_TYPES = ["Object", "Lamp"]
DEFAULT_SANITARY_NAMES = "WC; Toilet; Basin; Washbasin; Sink; Shower; Bath; Urinal; Bidet"
SHOWN_PER_CHECK = 2000
CHECKS = [
    # key, title, highlight color
    ("symbol_empty", "Symbols with an empty circuit, board or number", ERROR_COLOR),
    ("symbol_board", "Symbols naming a board that does not exist", ERROR_COLOR),
    ("device_unconnected", "Electrical devices not on a circuit (Archicad 30)", ERROR_COLOR),
    ("circuit_empty", "Circuits without devices (Archicad 30)", WARNING_COLOR),
    ("sanitary_outside", "Sanitary objects outside every zone", WARNING_COLOR),
]
ELECTRICAL_CHECKS = {"device_unconnected", "circuit_empty"}


def split_names(text: str) -> list[str]:
    """ The names of a ; or , separated list, without the empty ones. """
    return [name.strip() for name in re.split(r"[;,]", text) if name.strip()]


def is_empty(value) -> bool:
    """ An empty GDL parameter value: no value, blank text, or 0 for a number. """
    if value is None:
        return True
    if isinstance(value, bool):
        return False
    if isinstance(value, (int, float)):
        return value == 0
    if isinstance(value, list):
        return all(is_empty(v) for v in value)
    return not str(value).strip()


def value_text(value) -> str:
    if isinstance(value, float) and value.is_integer():
        return str(int(value))
    return str(value).strip()


def point_in_ring(x: float, y: float, ring: list[dict]) -> bool:
    """ Even-odd test; the arcs of the ring are taken as straight edges. """
    inside = False
    count = len(ring)
    for i in range(count):
        a, b = ring[i], ring[(i + 1) % count]
        if (a["y"] > y) != (b["y"] > y):
            crossing = a["x"] + (y - a["y"]) * (b["x"] - a["x"]) / (b["y"] - a["y"])
            if x < crossing:
                inside = not inside
    return inside


class ElectricalPlumbingCheck:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Finds electrical and plumbing symbols that miss data. Name the GDL parameters your symbols "
            "use for the circuit, the board and the number (or pick them from a selected symbol) and press "
            "Run checks. Select a group or a row to highlight it; double-click a row or press Select in "
            "Archicad to select it. Nothing in the model is changed.",
            min_width=820, min_height=680)
        self.enabled = {key: tk.BooleanVar(value=True) for key, _, _ in CHECKS}
        self.results: dict[str, list[dict]] = {}
        self.notes: dict[str, str] = {}
        self.rows: dict[str, tuple[str, int | None]] = {}
        self.stories: dict[int, str] = {}
        self.layer_names: dict[int, str] = {}
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_chosen)
        self.window.add_button("Run checks", self.run_checks, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Checks", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(1, weight=1)
        for index, (key, title, _) in enumerate(CHECKS):
            row, column = index // 2, (index % 2) * 2
            ttk.Checkbutton(options, text=title, variable=self.enabled[key]).grid(
                row=row, column=column, columnspan=2 if column == 0 else 1, sticky="w", pady=1,
                padx=(0 if column == 0 else 18, 0))

        fields = ttk.Frame(options)
        fields.grid(row=len(CHECKS) // 2 + 1, column=0, columnspan=4, sticky="ew", pady=(8, 0))
        fields.columnconfigure(1, weight=1)
        self.circuit_parameter = tk.StringVar()
        self.board_parameter = tk.StringVar()
        self.number_parameter = tk.StringVar()
        self.symbol_names = tk.StringVar()
        self.other_boards = tk.StringVar()
        self.sanitary_names = tk.StringVar(value=DEFAULT_SANITARY_NAMES)
        self.parameter_boxes = []
        rows = [
            ("Circuit parameter:", self.circuit_parameter, True),
            ("Board parameter:", self.board_parameter, True),
            ("Number parameter:", self.number_parameter, True),
            ("Symbols whose library part name contains:", self.symbol_names, False),
            ("Boards besides the Archicad 30 panels:", self.other_boards, False),
            ("Sanitary library part names contain:", self.sanitary_names, False),
        ]
        for row, (caption, variable, is_parameter) in enumerate(rows):
            ttk.Label(fields, text=caption).grid(row=row, column=0, sticky="w", pady=1)
            if is_parameter:
                box = ttk.Combobox(fields, textvariable=variable, width=40)
                self.parameter_boxes.append(box)
            else:
                box = ttk.Entry(fields, textvariable=variable, width=40)
            box.grid(row=row, column=1, sticky="ew", padx=(6, 0), pady=1)
        ttk.Button(fields, text="Pick from selection", command=self.window.guarded(self.load_parameter_names)).grid(
            row=0, column=2, rowspan=3, sticky="ns", padx=(8, 0), pady=1)
        ttk.Label(fields, style="Muted.TLabel",
                  text="Parameters are matched by name or by the name shown in the settings dialog. Leave a parameter "
                       "empty to skip it; a number parameter set to 0 counts as empty. Lists are separated by ; and "
                       "an empty symbol name list checks every object that has one of the parameters.",
                  wraplength=700, justify="left").grid(row=len(rows), column=0, columnspan=3, sticky="w", pady=(4, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("symbols", "symbols checked"), ("devices", "electrical devices"),
                                                 ("sanitary", "sanitary objects"), ("found", "findings")]):
            summary.columnconfigure(column, weight=1)
            value = ttk.Label(summary, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(summary, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        self.tree = make_tree(body, [
            ("type", "Library part / type", 180, "w"),
            ("story", "Story", 110, "w"),
            ("layer", "Layer", 120, "w"),
            ("note", "Note", 220, "w"),
        ], height=14, show_tree=True)
        self.tree.heading("#0", text="Check / Element ID", anchor="w")
        self.tree.column("#0", width=260, stretch=True)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("found", foreground="#c62828")
        self.tree.tag_configure("ok", foreground="#1b7f3b")
        self.tree.tag_configure("skipped", foreground="#777777")
        self.tree.tag_configure("more", foreground="#777777")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._highlight)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- parameter names ----

    def load_parameter_names(self) -> None:
        """ Offers the GDL parameter names of the selected symbols in the parameter boxes. """
        selected = get_elements("selection", SYMBOL_TYPES)
        if not selected:
            self.window.set_status("Select one or more symbols (objects or lamps) in Archicad first.", "warning")
            return
        names: dict[str, str] = {}
        for chunk in batched(selected[:50]):
            result = run_tapir("GetGDLParametersOfElements", {"elements": chunk}).get("gdlParametersOfElements", [])
            for entry in result:
                for parameter in entry.get("parameters", []) if isinstance(entry, dict) else []:
                    if parameter.get("type") in ("Title", "Separator"):
                        continue
                    name = parameter.get("name", "")
                    display = (parameter.get("displayName") or "").strip()
                    names.setdefault(name, f"{name}  ({display})" if display and display != name else name)
        values = [names[n] for n in sorted(names, key=str.lower)]
        for box in self.parameter_boxes:
            box.configure(values=values)
        self.window.set_status(f"{len(values)} parameter name(s) offered in the parameter lists.", "ok")

    @staticmethod
    def _parameter_name(text: str) -> str:
        # a picked entry is "name  (display name)"
        return text.split("  (", 1)[0].strip()

    # ---- checking ----

    def run_checks(self) -> None:
        on = {key for key, var in self.enabled.items() if var.get()}
        if not on:
            self.window.set_status("Choose at least one check.", "warning")
            return
        chosen = {
            "circuit": self._parameter_name(self.circuit_parameter.get()),
            "board": self._parameter_name(self.board_parameter.get()),
            "number": self._parameter_name(self.number_parameter.get()),
        }
        chosen = {role: name for role, name in chosen.items() if name}
        if on & {"symbol_empty", "symbol_board"} and not chosen:
            self.window.set_status("Name at least one GDL parameter for the symbol checks, or turn them off.", "warning")
            return

        self.results, self.notes = {}, {}
        counts = {"symbols": "–", "devices": "–", "sanitary": "–"}
        with self.window.busy("Reading the stories and layers..."):
            self.stories = {s["index"]: s.get("name") or f"Story {s['index']}"
                            for s in run_tapir("GetStories").get("stories", [])}
            layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
            self.layer_names = {int(a["index"]): a["name"] for a in layers}

            panels, circuits, devices, electrical_error = [], [], [], None
            if on & ELECTRICAL_CHECKS or "symbol_board" in on:
                self.window.set_status("Reading the electrical panels, circuits and devices...")
                try:
                    panels = run_tapir("GetElectricalPanels").get("electricalPanels", [])
                    circuits = run_tapir("GetElectricalCircuits").get("electricalCircuits", [])
                    devices = run_tapir("GetElectricalDevices").get("electricalDevices", [])
                except ArchicadError as e:
                    electrical_error = str(e)
                panels = [p for p in panels if "error" not in p]
                circuits = [c for c in circuits if "error" not in c]
                devices = [d for d in devices if "error" not in d]

            objects = []
            if on & {"symbol_empty", "symbol_board", "sanitary_outside"}:
                self.window.set_status("Reading the objects...")
                elements = get_elements("project", SYMBOL_TYPES)
                details = get_details_of_elements(elements, ["type", "id", "floorIndex", "layerIndex", "details",
                                                             "hotlinkId"])
                objects = [self._item(e["elementId"], d) for e, d in zip(elements, details)
                           if "error" not in d and "hotlinkId" not in d]

            if on & {"symbol_empty", "symbol_board"}:
                self.window.set_status("Reading the GDL parameters of the symbols...")
                counts["symbols"] = self._check_symbols(objects, chosen, panels, on)
            if on & ELECTRICAL_CHECKS:
                if electrical_error is not None:
                    print(f"Electrical checks skipped: {electrical_error}")
                    for key in on & ELECTRICAL_CHECKS:
                        self.notes[key] = "needs Archicad 30"
                else:
                    counts["devices"] = len(devices)
                    self._check_electrical(devices, circuits, panels, on)
            if "sanitary_outside" in on:
                self.window.set_status("Checking the sanitary objects against the zones...")
                counts["sanitary"] = self._check_sanitary(objects)

        self._show(counts)
        found = sum(len(f) for f in self.results.values())
        if found:
            self.window.set_status(f"Done: {found} finding(s).", "warning")
        else:
            self.window.set_status("Done: nothing is missing.", "ok")

    def _item(self, element_id: dict, detail: dict) -> dict:
        specific = detail.get("details") or {}
        origin = specific.get("origin") or {}
        return {
            "elementId": element_id,
            "type": detail.get("type", ""),
            "id": detail.get("id", ""),
            "floorIndex": detail.get("floorIndex"),
            "layerIndex": int(detail.get("layerIndex", -1)),
            "libPart": (specific.get("libPart") or {}).get("name", ""),
            "x": origin.get("x"),
            "y": origin.get("y"),
        }

    def _finding(self, item: dict, note: str) -> dict:
        return {
            "label": item.get("id") or "(no ID)",
            "type": item.get("libPart") or item.get("type", ""),
            "story": self.stories.get(item.get("floorIndex"), "" if item.get("floorIndex") is None
                                      else str(item.get("floorIndex"))),
            "layer": self.layer_names.get(item.get("layerIndex"), ""),
            "note": note,
            "elementId": item["elementId"],
        }

    def _check_symbols(self, objects: list[dict], chosen: dict[str, str], panels: list[dict], on: set[str]) -> int:
        name_parts = [n.lower() for n in split_names(self.symbol_names.get())]
        candidates = [o for o in objects if not name_parts or any(p in o["libPart"].lower() for p in name_parts)]
        boards = {p.get("panelName", "").strip().lower() for p in panels} | {
            n.lower() for n in split_names(self.other_boards.get())}
        boards.discard("")
        if "symbol_board" in on and ("board" not in chosen or not boards):
            self.notes["symbol_board"] = ("no board parameter named" if "board" not in chosen
                                          else "no boards known: no Archicad 30 panels and no board names given")

        wanted = {name.lower(): role for role, name in chosen.items()}
        empty, unknown_board, checked = [], [], 0
        for chunk in batched(candidates):
            result = run_tapir("GetGDLParametersOfElements", {
                "elements": [{"elementId": o["elementId"]} for o in chunk]}).get("gdlParametersOfElements", [])
            for item, entry in zip(chunk, result):
                if not isinstance(entry, dict) or "error" in entry:
                    continue
                values: dict[str, object] = {}
                for parameter in entry.get("parameters", []):
                    for key in (parameter.get("name", ""), parameter.get("displayName") or ""):
                        role = wanted.get(key.strip().lower())
                        if role and role not in values:
                            values[role] = parameter.get("value")
                if not values:
                    continue  # not a symbol that uses these parameters
                checked += 1
                missing = [role for role in ("circuit", "board", "number") if role in values and is_empty(values[role])]
                if missing:
                    empty.append(self._finding(item, f"empty {', '.join(missing)}"))
                if boards and "board" in values and not is_empty(values["board"]):
                    board = value_text(values["board"])
                    if board.lower() not in boards:
                        unknown_board.append(self._finding(item, f"board {board}"))
        if "symbol_empty" in on:
            self.results["symbol_empty"] = empty
        if "symbol_board" in on and "symbol_board" not in self.notes:
            self.results["symbol_board"] = unknown_board
        return checked

    def _electrical_items(self, element_ids: list[dict]) -> list[dict]:
        details = get_details_of_elements([{"elementId": e} for e in element_ids],
                                          ["type", "id", "floorIndex", "layerIndex"]) if element_ids else []
        items = []
        for element_id, detail in zip(element_ids, details):
            detail = detail if "error" not in detail else {}
            items.append({
                "elementId": element_id,
                "type": detail.get("type", ""),
                "id": detail.get("id", ""),
                "floorIndex": detail.get("floorIndex"),
                "layerIndex": int(detail.get("layerIndex", -1)),
            })
        return items

    def _check_electrical(self, devices: list[dict], circuits: list[dict], panels: list[dict], on: set[str]) -> None:
        if "device_unconnected" in on:
            loose = [d["elementId"] for d in devices if not d.get("parentCircuitId")]
            self.results["device_unconnected"] = [self._finding(item, "no circuit")
                                                 for item in self._electrical_items(loose)]
        if "circuit_empty" in on:
            panel_names = {p["elementId"]["guid"]: p.get("panelName", "") for p in panels}
            empty = [c for c in circuits if not c.get("childDevices") and not c.get("childPanels")]
            owners = self._electrical_items([c["ownerPanelId"] for c in empty])
            findings = []
            for circuit, owner in zip(empty, owners):
                panel = panel_names.get(circuit["ownerPanelId"]["guid"], "")
                finding = self._finding(owner, f"panel {panel}" if panel else "no devices")
                finding["label"] = circuit.get("name") or "(no name)"
                finding["type"] = "Circuit"
                findings.append(finding)
            self.results["circuit_empty"] = findings

    def _check_sanitary(self, objects: list[dict]) -> int:
        name_parts = [n.lower() for n in split_names(self.sanitary_names.get())]
        if not name_parts:
            self.notes["sanitary_outside"] = "no sanitary library part names given"
            return 0
        sanitary = [o for o in objects if any(p in o["libPart"].lower() for p in name_parts)]
        zones = get_elements("project", ["Zone"])
        zone_details = get_details_of_elements(zones, ["floorIndex", "details"])
        outlines: dict[int, list[tuple[list[dict], list[list[dict]]]]] = {}
        for detail in zone_details:
            specific = detail.get("details") or {}
            outline = specific.get("polygonOutline") or []
            if "error" in detail or len(outline) < 3:
                continue
            holes = [h.get("polygonOutline") or [] for h in specific.get("holes") or []]
            outlines.setdefault(detail.get("floorIndex"), []).append((outline, [h for h in holes if len(h) >= 3]))

        def in_a_zone(item: dict) -> bool:
            for outline, holes in outlines.get(item["floorIndex"], []):
                if point_in_ring(item["x"], item["y"], outline) and not any(
                        point_in_ring(item["x"], item["y"], hole) for hole in holes):
                    return True
            return False

        self.results["sanitary_outside"] = [
            self._finding(item, "outside every zone of its story")
            for item in sanitary if item["x"] is not None and item["y"] is not None and not in_a_zone(item)]
        return len(sanitary)

    # ---- showing ----

    def _show(self, counts: dict) -> None:
        self.tree.delete(*self.tree.get_children())
        self.rows = {}
        for key, title, _ in CHECKS:
            if key in self.notes and key not in self.results:
                row = f"check:{key}"
                self.tree.insert("", "end", iid=row, text=f"–  {title}", values=("", "", "", f"skipped: {self.notes[key]}"),
                                 tags=("skipped",))
                continue
            if key not in self.results:
                continue
            findings = self.results[key]
            check_row = f"check:{key}"
            self.tree.insert("", "end", iid=check_row, text=f"{'✖' if findings else '✔'}  {title}", open=False,
                             values=("", "", "", f"{len(findings)} found" if findings else "none"),
                             tags=("found" if findings else "ok",))
            self.rows[check_row] = (key, None)
            for index, finding in enumerate(findings[:SHOWN_PER_CHECK]):
                row = f"{key}:{index}"
                self.tree.insert(check_row, "end", iid=row, text=finding["label"],
                                 values=(finding["type"], finding["story"], finding["layer"], finding["note"]))
                self.rows[row] = (key, index)
            if len(findings) > SHOWN_PER_CHECK:
                self.tree.insert(check_row, "end", text=f"... and {len(findings) - SHOWN_PER_CHECK} more "
                                                        "(select the group to select them all)", tags=("more",))
        for key in ("symbols", "devices", "sanitary"):
            self.stat_labels[key].configure(text=str(counts[key]))
        found = sum(len(f) for f in self.results.values())
        self.stat_labels["found"].configure(text=str(found), foreground="#c62828" if found else "#1b7f3b")
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
        color_of = {key: color for key, _, color in CHECKS}
        colors: dict[str, list[int]] = {}
        for key, finding in self._chosen():
            colors.setdefault(finding["elementId"]["guid"], color_of[key])
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

    def select_chosen(self) -> None:
        guids = dict.fromkeys(f["elementId"]["guid"] for _, f in self._chosen())
        if not guids:
            self.window.set_status("There are no elements to select. Run the checks first.", "warning")
            return
        select_elements([{"elementId": {"guid": guid}} for guid in guids])
        self.window.set_status(f"Selected {len(guids)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row in self.rows and self.rows[row][1] is not None:
            key, index = self.rows[row]
            finding = self.results[key][index]
            select_elements([{"elementId": finding["elementId"]}])
            self.window.set_status(f"Selected {finding['label']} in Archicad.", "ok")

    def run(self) -> None:
        self.window.run(on_start=lambda: self.window.set_status(
            "Name the parameters and press Run checks."), on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ElectricalPlumbingCheck().run()
