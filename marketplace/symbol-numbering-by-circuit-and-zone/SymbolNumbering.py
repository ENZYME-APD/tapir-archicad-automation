# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "element-data-properties"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Electrical and Plumbing Symbol Numbering

Numbers electrical and plumbing symbols with a pattern such as
{board}.{circuit}-{n} or {zone}-{n}, writes the number to the Element ID or to
a GDL parameter, and can place a label with the number at each symbol.

Two sources:
- GDL symbols (Objects and Lamps): the board and the circuit come from a GDL
  parameter or a property picked in the window; they are numbered by position
  (rows from top to bottom, left to right) or by their current ID.
- Archicad 30 electrical devices: the board and the circuit are the device's
  panel and circuit; they are numbered in circuit order.

{zone} is the number of the zone whose outline holds the symbol's insertion
point on the symbol's own story. A preview lists the old and the new numbers
and the symbols that cannot be numbered; nothing is written until you confirm.

For Archicad with the Tapir Add-On.
"""

import math
import re
import tkinter as tk
from tkinter import ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Electrical and Plumbing Symbol Numbering"
GDL_ELEMENT_TYPES = ["Object", "Lamp"]
SCOPES = {"Selection": "selection", "Current story": "story", "Whole project": "project"}
TOKENS = ("{board}", "{circuit}", "{zone}")
FROM_PARAMETER = "GDL parameter"
FROM_PROPERTY = "Property"
NONE_CHOICE = "(none)"
ARC_STEPS = 16
EPSILON = 1e-9


################################
# TEXT AND NUMBERS
################################

def text_of(value) -> str:
    """ A GDL parameter or property value as text; whole numbers without decimals (circuit 1, not 1.0). """
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, float) and value.is_integer():
        value = int(value)
    if isinstance(value, (list, dict)):
        return ""
    return "" if value is None else str(value).strip()


def natural_key(text: str) -> list:
    """ Sorts "C2" before "C10". """
    return [(0, int(part), "") if part.isdigit() else (1, 0, part.lower()) for part in re.split(r"(\d+)", text) if part]


def parameter_name(choice: str) -> str:
    """ The parameter name of a combobox entry ("name — Display name") or of a typed name. """
    return choice.split(" — ")[0].strip()


_LENGTH = re.compile(r"^\s*([0-9]+(?:[.,][0-9]*)?|[.,][0-9]+)\s*(mm|cm|m|ft|'|in|\"|)\s*$", re.IGNORECASE)
_METERS_PER_UNIT = {"": 1.0, "m": 1.0, "cm": 0.01, "mm": 0.001, "ft": 0.3048, "'": 0.3048, "in": 0.0254, '"': 0.0254}


def parse_length(text: str) -> float | None:
    """ A length in meters from "0.3", "30 cm", "300 mm", "1 ft", "1'", "12 in" or '12"'; None when not valid. """
    match = _LENGTH.match(text or "")
    if not match:
        return None
    return float(match.group(1).replace(",", ".")) * _METERS_PER_UNIT[match.group(2).lower()]


def fill_pattern(pattern: str, values: dict[str, str], number: str) -> str:
    """ Plain replacement, so braces the user types elsewhere in the pattern never raise. """
    for token in TOKENS:
        pattern = pattern.replace(token, values.get(token, ""))
    return pattern.replace("{n}", number)


################################
# GEOMETRY
################################

def arc_points(a: tuple, b: tuple, angle: float) -> list[tuple]:
    """ The points between a and b on the arc from a to b that turns by angle (counterclockwise when positive). """
    dx, dy = b[0] - a[0], b[1] - a[1]
    chord = math.hypot(dx, dy)
    if chord < EPSILON or abs(angle) < EPSILON:
        return []
    h = (chord / 2.0) / math.tan(angle / 2.0)
    cx, cy = (a[0] + b[0]) / 2.0 - dy / chord * h, (a[1] + b[1]) / 2.0 + dx / chord * h
    radius = math.hypot(a[0] - cx, a[1] - cy)
    start = math.atan2(a[1] - cy, a[0] - cx)
    return [(cx + radius * math.cos(start + angle * i / ARC_STEPS), cy + radius * math.sin(start + angle * i / ARC_STEPS))
            for i in range(1, ARC_STEPS)]


def read_contour(points: list[dict] | None, arcs: list[dict] | None) -> list[tuple]:
    """ The contour as points, its curved edges as short straight ones, without the repeated closing point. """
    raw = [(float(p["x"]), float(p["y"])) for p in points or []]
    arc_of_edge = {int(a.get("begIndex", -1)): float(a.get("arcAngle", 0.0)) for a in arcs or []
                   if int(a.get("endIndex", -1)) == int(a.get("begIndex", -1)) + 1}
    result = []
    for i, point in enumerate(raw):
        result.append(point)
        if i + 1 < len(raw) and i in arc_of_edge:
            result += arc_points(point, raw[i + 1], arc_of_edge[i])
    if len(result) > 1 and abs(result[0][0] - result[-1][0]) < EPSILON and abs(result[0][1] - result[-1][1]) < EPSILON:
        result.pop()
    return result


def inside(p: tuple, points: list[tuple]) -> bool:
    result = False
    for i, a in enumerate(points):
        b = points[(i + 1) % len(points)]
        if (a[1] > p[1]) != (b[1] > p[1]):
            x = a[0] + (p[1] - a[1]) * (b[0] - a[0]) / (b[1] - a[1])
            if p[0] < x:
                result = not result
    return result


def in_zone(p: tuple, zone: dict) -> bool:
    return (len(zone["outline"]) > 2 and inside(p, zone["outline"])
            and not any(len(hole) > 2 and inside(p, hole) for hole in zone["holes"]))


def position_order(items: list[dict], row_tolerance: float) -> list[dict]:
    """ Rows from top to bottom, each from left to right. A symbol joins a row when it is at most
    row_tolerance below the row's topmost symbol. """
    rows: list[list[dict]] = []
    for item in sorted(items, key=lambda i: (-i["point"][1], i["point"][0])):
        if rows and rows[-1][0]["point"][1] - item["point"][1] <= row_tolerance:
            rows[-1].append(item)
        else:
            rows.append([item])
    return [item for row in rows for item in sorted(row, key=lambda i: i["point"][0])]


################################
# SCRIPT
################################

class SymbolNumbering:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Numbers electrical and plumbing symbols with a pattern. In the pattern {board} is the board, "
            "{circuit} the circuit, {zone} the number of the zone the symbol stands in and {n} the running "
            "number, counted separately for every different text the other parts give. Press Read symbols, "
            "check the list, then press Write and confirm: nothing is changed before that.",
            min_width=900, min_height=680)
        self.items: list[dict] = []  # the symbols that can be numbered
        self.skipped: list[dict] = []  # the symbols that cannot, with the reason
        self.parameter_choices: list[str] = []
        self.properties: dict[str, dict] = {}  # combobox text -> propertyId
        self.read_target = ""  # the target the current values were read from
        self.read_tokens: set[str] = set()  # the parts of the pattern when the symbols were read
        self._build_body()
        self.window.add_button("Read symbols", self.read_symbols)
        self.write_button = self.window.add_button("Write", self.write, primary=True)
        self.write_button.state(["disabled"])
        # needs the Write button, so only after it exists
        self._source_changed()

    # ---- window ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=3)
        body.rowconfigure(5, weight=1)

        source = ttk.LabelFrame(body, text="Symbols", padding=(10, 6))
        source.grid(row=0, column=0, sticky="ew")
        source.columnconfigure(3, weight=1)
        self.source = tk.StringVar(value="gdl")
        ttk.Radiobutton(source, text="GDL symbols (Objects and Lamps)", value="gdl", variable=self.source,
                        command=self.window.guarded(self._source_changed)).grid(row=0, column=0, columnspan=2, sticky="w")
        ttk.Radiobutton(source, text="Archicad electrical devices (Archicad 30), in circuit order", value="devices",
                        variable=self.source, command=self.window.guarded(self._source_changed)).grid(
            row=0, column=2, columnspan=3, sticky="w", padx=(16, 0))
        ttk.Label(source, text="Symbols in:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.scope = tk.StringVar(value="Current story")
        ttk.Combobox(source, textvariable=self.scope, values=list(SCOPES), state="readonly", width=14).grid(
            row=1, column=1, sticky="w", padx=(6, 0), pady=(6, 0))
        ttk.Button(source, text="Read parameter and property names",
                   command=self.window.guarded(self.load_names)).grid(row=1, column=2, sticky="w", padx=(16, 0), pady=(6, 0))

        self.gdl_frame = ttk.Frame(source)
        self.gdl_frame.grid(row=2, column=0, columnspan=5, sticky="ew", pady=(6, 0))
        self.gdl_frame.columnconfigure(2, weight=1)
        self.circuit_kind, self.circuit_name = tk.StringVar(value=FROM_PARAMETER), tk.StringVar()
        self.board_kind, self.board_name = tk.StringVar(value=FROM_PARAMETER), tk.StringVar()
        self.name_boxes: list[tuple[tk.StringVar, ttk.Combobox]] = []
        for row, (label, kind, name) in enumerate((("Circuit from:", self.circuit_kind, self.circuit_name),
                                                   ("Board from (optional):", self.board_kind, self.board_name))):
            ttk.Label(self.gdl_frame, text=label).grid(row=row, column=0, sticky="w", pady=2)
            kind_box = ttk.Combobox(self.gdl_frame, textvariable=kind, values=[FROM_PARAMETER, FROM_PROPERTY],
                                    state="readonly", width=14)
            kind_box.grid(row=row, column=1, sticky="w", padx=(6, 0), pady=2)
            name_box = ttk.Combobox(self.gdl_frame, textvariable=name, width=40)
            name_box.grid(row=row, column=2, sticky="ew", padx=(6, 0), pady=2)
            self.name_boxes.append((kind, name_box))
            kind.trace_add("write", lambda *_, v=name: (v.set(""), self._fill_name_boxes()))

        numbering = ttk.LabelFrame(body, text="Numbering", padding=(10, 6))
        numbering.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        numbering.columnconfigure(5, weight=1)
        self.pattern = tk.StringVar(value="{board}.{circuit}-{n}")
        self.start = tk.StringVar(value="1")
        self.digits = tk.StringVar(value="1")
        self.order = tk.StringVar(value="position")
        self.row_tolerance = tk.StringVar(value="0.3 m")
        ttk.Label(numbering, text="Pattern:").grid(row=0, column=0, sticky="w")
        ttk.Entry(numbering, textvariable=self.pattern, width=26).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(numbering, text="Start at:").grid(row=0, column=2, sticky="w", padx=(16, 0))
        ttk.Spinbox(numbering, textvariable=self.start, from_=0, to=9999, width=6).grid(row=0, column=3, sticky="w", padx=(6, 0))
        ttk.Label(numbering, text="Digits:").grid(row=0, column=4, sticky="w", padx=(16, 0))
        ttk.Spinbox(numbering, textvariable=self.digits, from_=1, to=6, width=6).grid(row=0, column=5, sticky="w", padx=(6, 0))

        order_row = ttk.Frame(numbering)
        order_row.grid(row=1, column=0, columnspan=6, sticky="w", pady=(6, 0))
        ttk.Label(order_row, text="Order of GDL symbols:").pack(side="left")
        self.order_widgets = [
            ttk.Radiobutton(order_row, text="By position", value="position", variable=self.order),
            ttk.Radiobutton(order_row, text="By current ID", value="id", variable=self.order),
        ]
        for widget in self.order_widgets:
            widget.pack(side="left", padx=(8, 0))
        ttk.Label(order_row, text="Row tolerance:").pack(side="left", padx=(16, 0))
        tolerance_entry = ttk.Entry(order_row, textvariable=self.row_tolerance, width=8)
        tolerance_entry.pack(side="left", padx=(6, 0))
        self.order_widgets.append(tolerance_entry)

        target_row = ttk.Frame(numbering)
        target_row.grid(row=2, column=0, columnspan=6, sticky="ew", pady=(6, 0))
        self.target = tk.StringVar(value="id")
        self.target_parameter = tk.StringVar()
        ttk.Label(target_row, text="Write to:").pack(side="left")
        ttk.Radiobutton(target_row, text="Element ID", value="id", variable=self.target).pack(side="left", padx=(8, 0))
        ttk.Radiobutton(target_row, text="GDL parameter:", value="parameter", variable=self.target).pack(side="left", padx=(8, 0))
        self.target_box = ttk.Combobox(target_row, textvariable=self.target_parameter, width=30)
        self.target_box.pack(side="left", padx=(6, 0))
        self.place_labels = tk.BooleanVar(value=False)
        self.label_favorite = tk.StringVar()
        ttk.Checkbutton(target_row, text="Place a label with the number, favorite (optional):",
                        variable=self.place_labels, command=self._update_preview).pack(side="left", padx=(16, 0))
        ttk.Entry(target_row, textvariable=self.label_favorite, width=16).pack(side="left", padx=(6, 0))

        for var in (self.pattern, self.start, self.digits, self.order, self.row_tolerance):
            var.trace_add("write", lambda *_: self._update_preview())
        # these need a new read, not only a new preview
        for var in (self.scope, self.circuit_name, self.board_name, self.target, self.target_parameter):
            var.trace_add("write", lambda *_: self._clear())

        ttk.Label(body, text="Preview", style="Muted.TLabel").grid(row=2, column=0, sticky="w", pady=(8, 2))
        self.tree = make_tree(body, [
            ("new", "New number", 130, "w"),
            ("current", "Current", 130, "w"),
            ("story", "Story", 90, "w"),
            ("board", "Board", 90, "w"),
            ("circuit", "Circuit", 90, "w"),
            ("zone", "Zone", 70, "w"),
            ("status", "Status", 220, "w"),
        ], height=12)
        self.tree.container.grid(row=3, column=0, sticky="nsew")

        self.skipped_label = ttk.Label(body, text="Cannot be numbered:")
        self.skipped_label.grid(row=4, column=0, sticky="w", pady=(8, 2))
        self.skipped_tree = make_tree(body, [
            ("current", "Current", 150, "w"),
            ("story", "Story", 100, "w"),
            ("reason", "Reason", 380, "w"),
        ], height=5)
        self.skipped_tree.container.grid(row=5, column=0, sticky="nsew")

    def _source_changed(self) -> None:
        gdl = self.source.get() == "gdl"
        # the board, the circuit and the order of electrical devices come from Archicad
        for widget in self.order_widgets + [w for w in self.gdl_frame.winfo_children() if isinstance(w, ttk.Widget)]:
            widget.state(["!disabled"] if gdl else ["disabled"])
        self._clear()

    def _fill_name_boxes(self) -> None:
        for kind, box in self.name_boxes:
            values = self.parameter_choices if kind.get() == FROM_PARAMETER else list(self.properties)
            box.configure(values=[NONE_CHOICE] + values)
        self.target_box.configure(values=self.parameter_choices)

    # ---- reading ----

    def _symbols(self) -> list[dict]:
        """ The symbols of the chosen source in the chosen scope, as {"elementId": ...}. """
        scope = SCOPES[self.scope.get()]
        if self.source.get() == "gdl":
            return get_elements(scope, GDL_ELEMENT_TYPES)
        return self.devices(scope)[0]

    def devices(self, scope: str) -> tuple[list[dict], list[dict]]:
        """ The electrical devices in the scope, and their details from GetElectricalDevices. """
        try:
            devices = run_tapir("GetElectricalDevices").get("electricalDevices", [])
        except ArchicadError as e:
            raise ArchicadError(f"{e} For older Archicad versions use the GDL symbols.") from e
        devices = [d for d in devices if "error" not in d]
        if scope == "selection":
            selected = {e["elementId"]["guid"] for e in run_tapir("GetSelectedElements").get("elements", [])}
            devices = [d for d in devices if d["elementId"]["guid"] in selected]
        elif scope == "story":
            current = run_tapir("GetStories").get("actStory", 0)
            details = get_details_of_elements([{"elementId": d["elementId"]} for d in devices], ["floorIndex"])
            devices = [d for d, detail in zip(devices, details) if detail.get("floorIndex") == current]
        return [{"elementId": d["elementId"]} for d in devices], devices

    def _read_parameters(self, elements: list[dict]) -> list[dict]:
        """ The GDL parameters of each element as {name: value}; {} for an element that has none. """
        result = []
        for chunk in batched(elements, 200):
            try:
                answer = run_tapir("GetGDLParametersOfElements", {"elements": chunk}).get("gdlParametersOfElements", [])
            except ArchicadError:
                answer = []
            answer += [{}] * (len(chunk) - len(answer))
            result += [{p["name"]: p.get("value") for p in a.get("parameters", [])} if "error" not in a else {} for a in answer]
        return result

    def _read_property(self, elements: list[dict], property_id: dict) -> list[str]:
        values = []
        for chunk in batched(elements):
            answer = run_tapir("GetPropertyValuesOfElements", {
                "elements": chunk, "properties": [{"propertyId": property_id}]}).get("propertyValuesForElements", [])
            answer += [{}] * (len(chunk) - len(answer))
            for entry in answer:
                found = (entry.get("propertyValues") or [{}])[0]
                values.append(text_of(found["propertyValue"].get("value")) if "propertyValue" in found else "")
        return values

    def load_names(self) -> None:
        """ Fills the parameter lists from the symbols in the scope, and the property list from the project. """
        with self.window.busy("Reading the parameter and property names..."):
            elements = self._symbols()
            names: dict[str, str] = {}
            for chunk in batched(elements[:300], 200):
                try:
                    answer = run_tapir("GetGDLParametersOfElements", {"elements": chunk}).get("gdlParametersOfElements", [])
                except ArchicadError:
                    continue
                for parameters in answer:
                    for parameter in parameters.get("parameters", []):
                        names.setdefault(parameter["name"], parameter.get("displayName") or "")
            properties = run_tapir("GetAllProperties", {"includeAvailability": False}).get("properties", [])
        self.parameter_choices = [f"{name} — {display}" if display and display != name else name
                                  for name, display in sorted(names.items(), key=lambda item: item[0].lower())]
        self.properties = {}
        for p in sorted((p for p in properties if "propertyId" in p),
                        key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                                       p.get("propertyName", "").lower())):
            self.properties[f"{p.get('propertyGroupName', '')} / {p.get('propertyName', '')}"] = p["propertyId"]
        self._fill_name_boxes()
        if elements:
            self.window.set_status(f"Read the parameter names of {min(len(elements), 300)} symbol(s) and "
                                   f"{len(self.properties)} properties. Pick the circuit, then press Read symbols.", "ok")
        else:
            self.window.set_status("There are no symbols in " + self.scope.get().lower() + ".", "warning")

    def _source_values(self, elements: list[dict], kind: str, choice: str, parameters: list[dict]) -> list[str] | None:
        """ The board or circuit of each element from the picked parameter or property; None when none is picked. """
        choice = choice.strip()
        if not choice or choice == NONE_CHOICE:
            return None
        if kind == FROM_PROPERTY:
            if choice not in self.properties:
                raise ArchicadError(f"There is no property \"{choice}\". Press Read parameter and property names "
                                    "and pick one from the list.")
            return self._read_property(elements, self.properties[choice])
        name = parameter_name(choice)
        return [text_of(p.get(name)) for p in parameters]

    def _zones(self) -> list[dict]:
        zones = run_tapir("GetElementsByType", {"elementType": "Zone"}).get("elements", [])
        result = []
        for detail in get_details_of_elements(zones, ["floorIndex", "details"]):
            d = detail.get("details") or {}
            if "error" in detail or "polygonOutline" not in d:
                continue
            result.append({
                "floorIndex": detail.get("floorIndex", 0),
                "number": (d.get("numberStr") or "").strip(),
                "outline": read_contour(d.get("polygonOutline"), d.get("polygonArcs")),
                "holes": [read_contour(h.get("polygonOutline"), h.get("polygonArcs")) for h in d.get("holes") or []],
            })
        return result

    def read_symbols(self) -> None:
        self.items, self.skipped = [], []
        target_parameter = parameter_name(self.target_parameter.get()) if self.target.get() == "parameter" else ""
        if self.target.get() == "parameter" and not target_parameter:
            self._update_preview()
            self.window.set_status("Pick the GDL parameter to write the numbers to.", "warning")
            return
        gdl = self.source.get() == "gdl"
        for token, choice in (("{circuit}", self.circuit_name), ("{board}", self.board_name)):
            if gdl and token in self.pattern.get() and choice.get().strip() in ("", NONE_CHOICE):
                self._update_preview()
                self.window.set_status(f"Pick where the {token[1:-1]} comes from, or take {token} out of the pattern.",
                                       "warning")
                return

        with self.window.busy("Reading the symbols..."):
            scope = SCOPES[self.scope.get()]
            if gdl:
                elements = get_elements(scope, GDL_ELEMENT_TYPES)
                devices = []
            else:
                elements, devices = self.devices(scope)
            # the position of a GDL symbol is in its details; a device's is its anchor point
            details = get_details_of_elements(elements, ["id", "floorIndex", "details"] if gdl else ["id", "floorIndex"])
            needs_parameters = bool(target_parameter) or (gdl and FROM_PARAMETER in (self.circuit_kind.get(), self.board_kind.get()))
            parameters = self._read_parameters(elements) if needs_parameters else [{}] * len(elements)
            if gdl:
                circuits = self._source_values(elements, self.circuit_kind.get(), self.circuit_name.get(), parameters)
                boards = self._source_values(elements, self.board_kind.get(), self.board_name.get(), parameters)
                order_in_circuit = [0] * len(elements)
            else:
                circuits, boards, order_in_circuit = self._device_circuits(devices)
            zones = self._zones() if "{zone}" in self.pattern.get() else []
            stories = run_tapir("GetStories").get("stories", [])
            story_names = get_story_names()

        if not elements:
            self._update_preview()
            self.window.set_status("There are no symbols in " + self.scope.get().lower() + ".", "warning")
            return

        pattern = self.pattern.get()
        for index, (element, detail) in enumerate(zip(elements, details)):
            item = {"elementId": element["elementId"], "currentId": detail.get("id", ""),
                    "floorIndex": detail.get("floorIndex"), "story": story_names.get(detail.get("floorIndex"), "")}
            if "error" in detail:
                item["reason"] = "cannot be read: " + item_errors([detail])[0]
                self.skipped.append(item)
                continue
            if gdl:
                origin = (detail.get("details") or {}).get("origin")
            else:
                origin = devices[index].get("anchorPoint")
            item["point"] = (float(origin["x"]), float(origin["y"])) if origin else (0.0, 0.0)
            if item["floorIndex"] is None and origin is not None:
                item["floorIndex"] = self._story_at(stories, float(origin.get("z", 0.0)))
            item["story"] = story_names.get(item["floorIndex"], "")
            item["current"] = text_of(parameters[index].get(target_parameter)) if target_parameter else item["currentId"]
            if target_parameter and target_parameter not in parameters[index]:
                item["reason"] = f"has no parameter {target_parameter}"
                self.skipped.append(item)
                continue
            item["circuit"] = circuits[index] if circuits is not None else ""
            item["board"] = boards[index] if boards is not None else ""
            item["circuitOrder"] = order_in_circuit[index]
            if "{circuit}" in pattern and not item["circuit"]:
                item["reason"] = "no circuit" if gdl else "not connected to a circuit"
                self.skipped.append(item)
                continue
            if "{board}" in pattern and not item["board"]:
                item["reason"] = "no board" if gdl or not item["circuit"] else "its circuit has no board"
                self.skipped.append(item)
                continue
            item["zone"] = ""
            if "{zone}" in pattern:
                if origin is None:
                    item["reason"] = "its position cannot be read"
                    self.skipped.append(item)
                    continue
                found = [z for z in zones if z["floorIndex"] == item["floorIndex"] and in_zone(item["point"], z)]
                if not found:
                    item["reason"] = "outside every zone"
                elif len(found) > 1:
                    item["reason"] = f"inside {len(found)} zones: " + ", ".join(z["number"] or "(no number)" for z in found)
                elif not found[0]["number"]:
                    item["reason"] = "its zone has no number"
                else:
                    item["zone"] = found[0]["number"]
                if "reason" in item:
                    self.skipped.append(item)
                    continue
            self.items.append(item)

        self.skipped.sort(key=lambda i: (i["story"], natural_key(i["currentId"])))
        self.read_target = target_parameter
        self.read_tokens = {token for token in TOKENS if token in pattern}
        self._update_preview()

    @staticmethod
    def _story_at(stories: list[dict], z: float) -> int:
        below = [s for s in stories if s.get("level", 0.0) <= z + 1e-6]
        return max(below, key=lambda s: s["level"])["index"] if below else 0

    def _device_circuits(self, devices: list[dict]) -> tuple[list[str], list[str], list[int]]:
        """ The circuit name, the panel name and the place in its circuit of each device. """
        panels = run_tapir("GetElectricalPanels").get("electricalPanels", [])
        circuits = run_tapir("GetElectricalCircuits").get("electricalCircuits", []) if panels else []
        panel_names = {p["elementId"]["guid"]: p.get("panelName", "") for p in panels if "error" not in p}
        circuit_of: dict[str, tuple[str, str, int]] = {}
        for circuit in circuits:
            if "error" in circuit:
                continue
            board = panel_names.get(circuit["ownerPanelId"]["guid"], "")
            for place, device in enumerate(circuit.get("childDevices", [])):
                circuit_of[device["elementId"]["guid"]] = (circuit.get("name", ""), board, place)
        names, boards, places = [], [], []
        for device in devices:
            name, board, place = circuit_of.get(device["elementId"]["guid"], ("", "", 0))
            names.append(name)
            boards.append(board)
            places.append(place)
        return names, boards, places

    # ---- numbering ----

    def _start_number(self) -> int | None:
        try:
            value = int(self.start.get())
        except ValueError:
            return None
        return value if value >= 0 else None

    def _digits(self) -> int:
        try:
            return max(1, min(6, int(self.digits.get())))
        except ValueError:
            return 1

    def _assign_numbers(self) -> str | None:
        """ Orders the items and fills in each one's new number; the problem when the options are not valid. """
        start = self._start_number()
        if start is None:
            return "The start number must be a whole number, 0 or more."
        tolerance = parse_length(self.row_tolerance.get())
        if tolerance is None:
            return "The row tolerance must be a length, e.g. 0.3 m, 30 cm or 1 ft."
        pattern = self.pattern.get()
        if "{n}" not in pattern:
            return "Put {n} in the pattern: it is the running number."
        new_tokens = [token for token in TOKENS if token in pattern and token not in self.read_tokens]
        if (self.items or self.skipped) and new_tokens:
            # the symbols were checked only for the parts the pattern had when they were read
            return f"The pattern now uses {', '.join(new_tokens)}: press Read symbols again."
        digits = self._digits()
        groups: dict[str, list[dict]] = {}
        for item in self.items:
            item["values"] = {"{board}": item["board"], "{circuit}": item["circuit"], "{zone}": item["zone"]}
            groups.setdefault(fill_pattern(pattern, item["values"], "\0"), []).append(item)

        ordered = []
        for key in sorted(groups, key=natural_key):
            group = groups[key]
            if self.source.get() == "devices":
                group.sort(key=lambda i: (natural_key(i["board"]), natural_key(i["circuit"]), i["circuitOrder"]))
            elif self.order.get() == "id":
                group.sort(key=lambda i: natural_key(i["currentId"]))
            else:
                group = position_order(group, tolerance)
            for offset, item in enumerate(group):
                item["new"] = fill_pattern(pattern, item["values"], f"{start + offset:0{digits}d}")
            ordered += group
        self.items = ordered
        return None

    def _duplicates(self) -> set[str]:
        seen, duplicates = set(), set()
        for item in self.items:
            (duplicates if item["new"] in seen else seen).add(item["new"])
        return duplicates

    def _to_label(self) -> list[dict]:
        return [item for item in self.items if not item.get("labelled")] if self.place_labels.get() else []

    def _clear(self) -> None:
        self.items, self.skipped = [], []
        self._update_preview()
        self.window.set_status("Press Read symbols to read the symbols.")

    def _update_preview(self) -> None:
        self.tree.delete(*self.tree.get_children())
        self.skipped_tree.delete(*self.skipped_tree.get_children())
        self.write_button.state(["disabled"])
        problem = self._assign_numbers()
        for index, item in enumerate(self.skipped):
            self.skipped_tree.insert("", "end", iid=str(index), values=(
                item["currentId"] or "(empty)", item.get("story", ""), item["reason"]))
        self.skipped_label.configure(text=f"Cannot be numbered: {len(self.skipped)}")
        if problem:
            self.window.set_status(problem, "warning")
            return

        duplicates = self._duplicates() if not self.read_target else set()
        to_change = 0
        for index, item in enumerate(self.items):
            if item["new"] == item["current"]:
                status = "written" if item.get("writtenTo") == item["new"] else "unchanged"
            else:
                to_change += 1
                if item.get("failedTo") == item["new"]:
                    status = "not written: " + item["failure"]
                else:
                    status = "duplicate new number" if item["new"] in duplicates else ""
            if item.get("labelled"):
                status += ", labelled"
            elif item.get("labelFailure"):
                status += ", no label: " + item["labelFailure"]
            self.tree.insert("", "end", iid=str(index), values=(
                item["new"], item["current"] or "(empty)", item["story"], item["board"], item["circuit"],
                item["zone"], status.lstrip(", ")))

        if not self.items:
            if self.skipped:
                self.window.set_status(f"None of the {len(self.skipped)} symbol(s) can be numbered; the list below says why.",
                                       "warning")
            return
        labels = len(self._to_label())
        if to_change or labels:
            self.write_button.state(["!disabled"])
        message = f"{to_change} of {len(self.items)} symbol(s) get a new number."
        if labels:
            message += f" {labels} label(s) to place."
        if self.skipped:
            message += f" {len(self.skipped)} cannot be numbered."
        if duplicates:
            self.window.set_status(message + " Some new numbers repeat.", "warning")
        else:
            self.window.set_status(message)

    # ---- writing ----

    def write(self) -> None:
        if self._assign_numbers():
            return
        changes = [item for item in self.items if item["new"] != item["current"]]
        to_label = self._to_label()
        if not changes and not to_label:
            return
        target = f"the GDL parameter {self.read_target}" if self.read_target else "the Element ID"
        question = []
        if changes:
            question.append(f"Write the new number of {len(changes)} symbol(s) to {target}?")
            duplicates = self._duplicates() if not self.read_target else set()
            if duplicates:
                question.append(f"{len(duplicates)} new number(s) are used more than once.")
        if to_label:
            question.append(f"Place {len(to_label)} label(s) with the number.")
        question.append("This can be undone in Archicad with Undo: the numbers in one step"
                        + (", the labels in a second one." if changes and to_label else "."))
        if not self.window.ask_ok_cancel(SCRIPT_NAME, "\n\n".join(question)):
            return

        failed = 0
        if changes:
            # one call, so all the numbers are one undo step
            with self.window.busy(f"Writing {len(changes)} number(s)..."):
                if self.read_target:
                    results = run_tapir("SetGDLParametersOfElements", {"elementsWithGDLParameters": [
                        {"elementId": item["elementId"], "gdlParameters": [{"name": self.read_target, "value": item["new"]}]}
                        for item in changes]}).get("executionResults", [])
                else:
                    results = run_tapir("SetElementInfoStrings", {"elements": [
                        {"elementId": item["elementId"], "infoString": item["new"]} for item in changes
                    ]}).get("executionResults", [])
            results += [{"error": {"message": "no result"}}] * (len(changes) - len(results))
            for item, result in zip(changes, results):
                if "error" in result or result.get("success") is False:
                    failed += 1
                    item["failedTo"] = item["new"]
                    item["failure"] = item_errors([result])[0] if "error" in result else "not changed"
                else:
                    item["current"] = item["writtenTo"] = item["new"]
                    item.pop("failedTo", None)

        labelled = label_failed = 0
        to_label = [item for item in to_label if item["new"] == item["current"]]
        if to_label:
            favorite = self.label_favorite.get().strip()
            with self.window.busy(f"Placing {len(to_label)} label(s)..."):
                answer = run_tapir("CreateLabels", {"labelsData": [{
                    "parentElementId": item["elementId"], "labelClass": "Text", "text": item["new"],
                    **({"favoriteName": favorite} if favorite else {})} for item in to_label]}).get("elements", [])
            answer += [{"error": {"message": "no label created"}}] * (len(to_label) - len(answer))
            for item, created in zip(to_label, answer):
                if "error" in created:
                    label_failed += 1
                    item["labelFailure"] = item_errors([created])[0]
                else:
                    labelled += 1
                    item["labelled"] = True
                    item.pop("labelFailure", None)

        self._update_preview()
        parts = []
        if changes:
            parts.append(f"Wrote {len(changes) - failed} of {len(changes)} number(s).")
        if labelled or label_failed:
            parts.append(f"Placed {labelled} label(s).")
        if failed or label_failed:
            parts.append("The status in the list says why the others failed.")
            self.window.set_status(" ".join(parts), "warning")
        else:
            self.window.set_status(" ".join(parts), "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_names)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    SymbolNumbering().run()
