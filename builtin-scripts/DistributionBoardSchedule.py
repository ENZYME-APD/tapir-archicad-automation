# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "quantities-scheduling"
# changes = "files"
# ///

'''
Distribution board schedule

Lists the circuits of every distribution board with their loads, totals each
board and its phases, flags the circuits over a load limit and the boards whose
phases are out of balance, and saves the schedule as CSV or Excel. It only
reads the project.

Two sources:
- Archicad 30 electrical panels and circuits (GetElectricalPanels,
  GetElectricalCircuits, GetElectricalProjectSettings).
- GDL symbols (Objects and Lamps), grouped by the board and circuit parameters
  picked in the window, with the load summed from a load parameter and the
  phases read from an optional phase parameter (GetElementsByType,
  GetGDLParametersOfElements).
'''

import datetime
import os
import re
import tkinter as tk
from tkinter import filedialog, font as tkfont, ttk

from utilities.archicad import ArchicadError, batched, get_elements, item_errors, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Distribution Board Schedule"
GDL_ELEMENT_TYPES = ["Object", "Lamp"]
PHASES = ("A", "B", "C")
NO_VALUE = "(none)"
EXPORT_HEADERS = ["Board", "Circuit", "Description", "Phases", "Voltage", "Connected load",
                  "Load phase A", "Load phase B", "Load phase C", "Voltage drop", "Cable length", "Note"]


def fmt(value) -> str:
    """ A number with at most two decimals and no trailing zeros; anything else as it is. """
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return "" if value is None else str(value)
    text = f"{value:,.2f}".rstrip("0").rstrip(".")
    return "0" if text == "-0" else text


def to_float(text: str) -> float | None:
    try:
        return float(text.strip().replace(",", "."))
    except ValueError:
        return None


def number(value) -> float | None:
    """ A GDL parameter value as a number, None when it is not one. """
    if isinstance(value, bool):
        return float(value)
    if isinstance(value, (int, float)):
        return float(value)
    if isinstance(value, str):
        return to_float(value)
    return None


_PHASE_TOKEN = re.compile(r"(?:L[123]|[ABC123])+")
_PHASE_PART = re.compile(r"L[123]|[ABC123]")
_PHASE_OF_PART = {"A": "A", "B": "B", "C": "C", "1": "A", "2": "B", "3": "C", "L1": "A", "L2": "B", "L3": "C"}


def phases_of(value) -> list[str]:
    """ The phases named by a GDL phase parameter value: A, B and C, L1, L2 and L3 or 1, 2 and 3,
    alone or combined ("A", "L2", "ABC", "L1 L2 L3", "1,2,3", "Phase B"). Words that are not
    phase names are ignored. """
    if value is None or isinstance(value, (bool, list, dict)):
        return []
    if isinstance(value, float) and value.is_integer():
        value = int(value)
    found = set()
    for token in re.split(r"[^A-Z0-9]+", str(value).upper()):
        if _PHASE_TOKEN.fullmatch(token):
            found.update(_PHASE_OF_PART[part] for part in _PHASE_PART.findall(token))
    return [phase for phase in PHASES if phase in found]


def imbalance(loads: list[float], used_phases: set[str]) -> float | None:
    """ The largest deviation of the load of a used phase from their average, in percent of the
    average. None when fewer than two phases are used (a single-phase board) or there is no load. """
    used = [loads[PHASES.index(phase)] for phase in PHASES if phase in used_phases]
    if len(used) < 2 or sum(used) <= 0:
        return None
    average = sum(used) / len(used)
    return max(abs(load - average) for load in used) / average * 100.0


def text_of(value) -> str:
    """ A GDL parameter value as a group name; whole numbers without decimals (circuit 1, not 1.0). """
    if isinstance(value, float) and value.is_integer():
        value = int(value)
    return "" if value is None else str(value).strip()


def parameter_name(choice: str) -> str:
    """ The parameter name of a combobox entry ("name — Display name") or of a typed name. """
    return choice.split(" — ")[0].strip()


class DistributionBoardSchedule:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the circuits of every distribution board with their loads, the board totals and the "
            "load per phase. Circuits over the load limit and boards whose phases differ from their "
            "average by more than the set percent are flagged. Nothing in the project is changed.",
            min_width=820, min_height=520)
        self.boards: list[dict] = []
        self.parameter_choices: list[str] = []
        self.read_status: tuple[str, str] | None = None
        self._build_body()
        self.window.add_button("Save as...", self.export)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        source = ttk.LabelFrame(body, text="Source", padding=(10, 6))
        source.grid(row=0, column=0, sticky="ew")
        source.columnconfigure(5, weight=1)
        self.source = tk.StringVar(value="electrical")
        ttk.Radiobutton(source, text="Archicad electrical panels and circuits (Archicad 30 or later)",
                        value="electrical", variable=self.source,
                        command=self.window.guarded(self._source_changed)).grid(row=0, column=0, columnspan=6, sticky="w")
        ttk.Radiobutton(source, text="GDL symbols, grouped by their parameters",
                        value="gdl", variable=self.source,
                        command=self.window.guarded(self._source_changed)).grid(row=1, column=0, columnspan=6, sticky="w")

        self.gdl_frame = ttk.Frame(source, padding=(20, 4, 0, 0))
        self.gdl_frame.grid(row=2, column=0, columnspan=6, sticky="ew")
        self.gdl_frame.columnconfigure(1, weight=1)
        self.gdl_frame.columnconfigure(3, weight=1)

        self.scope = tk.StringVar(value="project")
        scope_row = ttk.Frame(self.gdl_frame)
        scope_row.grid(row=0, column=0, columnspan=4, sticky="w")
        ttk.Label(scope_row, text="Symbols in:").pack(side="left")
        for text, value in (("Whole project", "project"), ("Current story", "story"), ("Selection", "selection")):
            ttk.Radiobutton(scope_row, text=text, value=value, variable=self.scope).pack(side="left", padx=(8, 0))
        ttk.Button(scope_row, text="Read parameter names",
                   command=self.window.guarded(self.load_parameter_names)).pack(side="left", padx=(16, 0))

        self.board_parameter = tk.StringVar()
        self.circuit_parameter = tk.StringVar()
        self.load_parameter = tk.StringVar()
        self.phase_parameter = tk.StringVar()
        self.parameter_boxes: list[ttk.Combobox] = []
        for index, (label, variable) in enumerate((
                ("Board parameter:", self.board_parameter), ("Circuit parameter:", self.circuit_parameter),
                ("Load parameter:", self.load_parameter), ("Phase parameter (optional):", self.phase_parameter))):
            row, column = 1 + index // 2, (index % 2) * 2
            ttk.Label(self.gdl_frame, text=label).grid(row=row, column=column, sticky="w", pady=(6, 0),
                                                       padx=(0 if column == 0 else 12, 0))
            box = ttk.Combobox(self.gdl_frame, textvariable=variable, width=28)
            box.grid(row=row, column=column + 1, sticky="ew", padx=(6, 0), pady=(6, 0))
            self.parameter_boxes.append(box)
        ttk.Label(self.gdl_frame, style="Muted.TLabel",
                  text="Phase values: A, B, C or L1, L2, L3 or 1, 2, 3; a three-phase symbol names all three "
                       "(e.g. ABC). Without a phase parameter the load per phase is not known.").grid(
            row=3, column=0, columnspan=4, sticky="w", pady=(6, 0))

        limits = ttk.LabelFrame(body, text="Flags", padding=(10, 6))
        limits.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.load_limit = tk.StringVar(value="")
        self.imbalance_limit = tk.StringVar(value="10")
        ttk.Label(limits, text="Circuit load over:").grid(row=0, column=0, sticky="w")
        ttk.Entry(limits, textvariable=self.load_limit, width=10).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(limits, text="(in the load's unit, empty: no limit)", style="Muted.TLabel").grid(
            row=0, column=2, sticky="w", padx=(4, 16))
        ttk.Label(limits, text="Phases out of balance by more than:").grid(row=0, column=3, sticky="w")
        ttk.Entry(limits, textvariable=self.imbalance_limit, width=6).grid(row=0, column=4, sticky="w", padx=(6, 0))
        ttk.Label(limits, text="%", style="Muted.TLabel").grid(row=0, column=5, sticky="w", padx=(4, 0))
        ttk.Button(limits, text="Apply", command=self.window.guarded(self.show)).grid(row=0, column=6, sticky="w", padx=(12, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=2, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Press Refresh to read the distribution boards.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("description", "Description", 160, "w"),
            ("phases", "Phases", 60, "w"),
            ("voltage", "Voltage", 60, "e"),
            ("load", "Connected load", 100, "e"),
            ("loadA", "Phase A", 70, "e"),
            ("loadB", "Phase B", 70, "e"),
            ("loadC", "Phase C", 70, "e"),
            ("drop", "Voltage drop", 85, "e"),
            ("length", "Cable length", 85, "e"),
            ("note", "Note", 200, "w"),
        ], height=12, show_tree=True)
        self.tree.heading("#0", text="Board / Circuit", anchor="w")
        self.tree.column("#0", width=180, stretch=True)
        # kept on self: Tk deletes the font when the Python object is garbage collected
        self.bold_font = tkfont.nametofont("TkDefaultFont").copy()
        self.bold_font.configure(weight="bold")
        self.tree.tag_configure("board", font=self.bold_font)
        self.tree.tag_configure("flagged", foreground="#c62828")
        self.tree.container.grid(row=3, column=0, sticky="nsew")

        self._source_changed()

    def _source_changed(self) -> None:
        state = "!disabled" if self.source.get() == "gdl" else "disabled"
        for child in self.gdl_frame.winfo_children():
            self._set_state(child, state)
        if self.source.get() == "gdl" and not self.parameter_choices:
            self.load_parameter_names()

    def _set_state(self, widget: tk.Misc, state: str) -> None:
        if isinstance(widget, ttk.Widget):
            widget.state([state])
        for child in widget.winfo_children():
            self._set_state(child, state)

    # Reading

    def _gdl_elements(self) -> list[dict]:
        return get_elements(self.scope.get(), GDL_ELEMENT_TYPES)

    def _read_gdl_parameters(self, elements: list[dict]) -> list[dict]:
        parameters = []
        for chunk in batched(elements, 200):
            parameters += run_tapir("GetGDLParametersOfElements", {"elements": chunk}).get("gdlParametersOfElements", [])
        return parameters

    def load_parameter_names(self) -> None:
        """ Fills the parameter lists with the parameters of the symbols in the scope. """
        with self.window.busy("Reading the parameter names of the symbols..."):
            elements = self._gdl_elements()
            names: dict[str, str] = {}
            for parameters in self._read_gdl_parameters(elements[:300]):
                for parameter in parameters.get("parameters", []):
                    names.setdefault(parameter["name"], parameter.get("displayName") or "")
        self.parameter_choices = [f"{name} — {display}" if display and display != name else name
                                  for name, display in sorted(names.items(), key=lambda item: item[0].lower())]
        for box in self.parameter_boxes:
            box.configure(values=self.parameter_choices)
        if elements:
            self.window.set_status(f"Read the parameter names of {min(len(elements), 300)} symbol(s). "
                                   "Pick the board, circuit and load parameters, then press Refresh.", "ok")
        else:
            self.window.set_status("No objects or lamps in the chosen scope.", "warning")

    def refresh(self) -> None:
        if self.source.get() == "electrical":
            self._read_electrical()
        else:
            self._read_gdl()
        self.show()

    def _project_voltage(self) -> float | None:
        try:
            return run_tapir("GetElectricalProjectSettings").get("voltage")
        except ArchicadError:
            return None

    def _read_electrical(self) -> None:
        with self.window.busy("Reading the electrical panels and circuits..."):
            try:
                panels = run_tapir("GetElectricalPanels").get("electricalPanels", [])
            except ArchicadError as e:
                self.boards = []
                raise ArchicadError(f"{e} For older Archicad versions use the GDL symbols source.") from e
            circuits = run_tapir("GetElectricalCircuits").get("electricalCircuits", []) if panels else []
            voltage = self._project_voltage()

        failed = item_errors(panels) + item_errors(circuits)
        by_panel: dict[str, list[dict]] = {}
        for circuit in circuits:
            if "error" not in circuit:
                by_panel.setdefault(circuit["ownerPanelId"]["guid"], []).append(circuit)
        names = {p["elementId"]["guid"]: p["panelName"] for p in panels if "error" not in p}

        self.boards = []
        for panel in panels:
            if "error" in panel:
                continue
            load = panel["connectedLoad"]
            fed_from = ""
            if "parentCircuitId" in panel:
                parent = next((c for c in circuits if "error" not in c
                               and c["elementId"]["guid"] == panel["parentCircuitId"]["guid"]), None)
                if parent is not None:
                    fed_from = f"Fed from {names.get(parent['ownerPanelId']['guid'], '?')} {parent['name']}"
            panel_circuits = by_panel.get(panel["elementId"]["guid"], [])
            self.boards.append({
                "name": panel["panelName"] or "(no name)",
                "description": fed_from,
                "total": load["total"],
                "phaseLoads": [load["phaseA"], load["phaseB"], load["phaseC"]],
                "usedPhases": {phase for c in panel_circuits for phase in PHASES if c[f"isPhase{phase}Used"]},
                "circuits": [{
                    "name": c["name"],
                    "description": c["description"],
                    "phases": c["phases"],
                    "voltage": c["voltage"],
                    "load": c["connectedLoad"]["total"],
                    "phaseLoads": [c["connectedLoad"]["phaseA"], c["connectedLoad"]["phaseB"], c["connectedLoad"]["phaseC"]],
                    "voltageDrop": c["voltageDrop"],
                    "cableLength": c["estimatedCableLength"],
                } for c in panel_circuits],
            })
        self.boards.sort(key=lambda b: b["name"].lower())

        detail = f"{len(self.boards)} electrical panel(s), {sum(len(b['circuits']) for b in self.boards)} circuit(s)"
        if voltage is not None:
            detail += f", project voltage {fmt(voltage)}"
        self.detail_label.configure(text=detail)
        self.read_status = (f"{len(failed)} panel(s) or circuit(s) could not be read: {failed[0]}", "warning") if failed else None

    def _read_gdl(self) -> None:
        board_name = parameter_name(self.board_parameter.get())
        circuit_name = parameter_name(self.circuit_parameter.get())
        load_name = parameter_name(self.load_parameter.get())
        phase_name = parameter_name(self.phase_parameter.get())
        if not board_name or not circuit_name or not load_name:
            raise ArchicadError("Pick the board, circuit and load parameters first.")

        with self.window.busy("Reading the GDL symbols..."):
            elements = self._gdl_elements()
            all_parameters = self._read_gdl_parameters(elements)
            voltage = self._project_voltage()

        boards: dict[str, dict[str, dict]] = {}
        used = 0
        not_numeric = 0
        for parameters in all_parameters:
            if "error" in parameters:
                continue
            values = {p["name"]: p.get("value") for p in parameters.get("parameters", [])}
            if board_name not in values or circuit_name not in values or load_name not in values:
                continue
            used += 1
            load = number(values[load_name])
            if load is None:
                not_numeric += 1
                load = 0.0
            board = text_of(values[board_name]) or NO_VALUE
            circuit_key = text_of(values[circuit_name]) or NO_VALUE
            circuit = boards.setdefault(board, {}).setdefault(circuit_key, {
                "name": circuit_key, "count": 0, "load": 0.0, "phaseLoads": [0.0, 0.0, 0.0],
                "phases": set(), "unphased": 0.0})
            circuit["count"] += 1
            circuit["load"] += load
            phases = phases_of(values.get(phase_name)) if phase_name else []
            if phases:
                circuit["phases"].update(phases)
                for phase in phases:
                    circuit["phaseLoads"][PHASES.index(phase)] += load / len(phases)
            else:
                circuit["unphased"] += load

        self.boards = []
        for board, circuits in boards.items():
            rows = []
            for circuit in sorted(circuits.values(), key=lambda c: c["name"].lower()):
                phase_known = not circuit["unphased"]
                rows.append({
                    "name": circuit["name"],
                    "description": f"{circuit['count']} symbol(s)",
                    "phases": "".join(sorted(circuit["phases"])),
                    "voltage": None,
                    "load": circuit["load"],
                    "phaseLoads": circuit["phaseLoads"] if phase_known else None,
                    "voltageDrop": None,
                    "cableLength": None,
                    "note": "" if phase_known or not phase_name else "phase missing on some symbols",
                })
            phase_known = all(r["phaseLoads"] is not None for r in rows)
            self.boards.append({
                "name": board,
                "description": "",
                "total": sum(r["load"] for r in rows),
                "phaseLoads": [sum(r["phaseLoads"][i] for r in rows) for i in range(3)] if phase_known else None,
                "usedPhases": {phase for circuit in circuits.values() for phase in circuit["phases"]},
                "circuits": rows,
            })
        self.boards.sort(key=lambda b: b["name"].lower())

        detail = (f"{used} of {len(elements)} object(s) and lamp(s) have the board, circuit and load parameters, "
                  f"in {len(self.boards)} board(s)")
        if voltage is not None:
            detail += f", project voltage {fmt(voltage)}"
        self.detail_label.configure(text=detail)
        failed = item_errors(all_parameters)
        problems = []
        if not_numeric:
            problems.append(f"{not_numeric} symbol(s) have a load that is not a number, counted as 0")
        if failed:
            problems.append(f"{len(failed)} symbol(s) could not be read: {failed[0]}")
        self.read_status = ("; ".join(problems) + ".", "warning") if problems else None

    # Showing

    def _limits(self) -> tuple[float | None, float | None]:
        load_limit = to_float(self.load_limit.get()) if self.load_limit.get().strip() else None
        imbalance_limit = to_float(self.imbalance_limit.get()) if self.imbalance_limit.get().strip() else None
        if self.load_limit.get().strip() and load_limit is None:
            self.load_limit.set("")
        if self.imbalance_limit.get().strip() and imbalance_limit is None:
            self.imbalance_limit.set("")
        return load_limit, imbalance_limit

    def _annotate(self) -> int:
        """ Sets the notes and flags of the boards and circuits; returns the number of flags. """
        load_limit, imbalance_limit = self._limits()
        flags = 0
        for board in self.boards:
            for circuit in board["circuits"]:
                notes = [circuit["note"]] if circuit.get("note") else []
                circuit["flagged"] = load_limit is not None and circuit["load"] > load_limit
                if circuit["flagged"]:
                    notes.insert(0, f"over the limit of {fmt(load_limit)}")
                    flags += 1
                circuit["noteText"] = "; ".join(notes)
            phase_loads = board["phaseLoads"]
            board["imbalance"] = imbalance(phase_loads, board["usedPhases"]) if phase_loads is not None else None
            board["flagged"] = (imbalance_limit is not None and board["imbalance"] is not None
                                and board["imbalance"] > imbalance_limit)
            if board["flagged"]:
                flags += 1
            if phase_loads is None:
                board["noteText"] = "load per phase not known"
            elif len(board["usedPhases"]) < 2:
                board["noteText"] = "single phase" if board["usedPhases"] else ""
            elif board["imbalance"] is None:
                board["noteText"] = "no load"
            else:
                board["noteText"] = (f"{'out of balance' if board['flagged'] else 'phase balance'}: "
                                     f"{board['imbalance']:.1f}% from the average")
        return flags

    def show(self) -> None:
        flags = self._annotate()
        self.tree.delete(*self.tree.get_children())
        for board_index, board in enumerate(self.boards):
            phase_loads = board["phaseLoads"] or ["", "", ""]
            board_iid = f"b{board_index}"
            self.tree.insert("", "end", iid=board_iid, text=board["name"], open=True,
                             tags=("board", "flagged") if board["flagged"] else ("board",), values=(
                                 board["description"], "", "", fmt(board["total"]),
                                 fmt(phase_loads[0]), fmt(phase_loads[1]), fmt(phase_loads[2]), "", "",
                                 board["noteText"]))
            for circuit in board["circuits"]:
                phase_loads = circuit["phaseLoads"] or ["", "", ""]
                self.tree.insert(board_iid, "end", text=circuit["name"],
                                 tags=("flagged",) if circuit["flagged"] else (), values=(
                                     circuit["description"], circuit["phases"], fmt(circuit["voltage"]),
                                     fmt(circuit["load"]), fmt(phase_loads[0]), fmt(phase_loads[1]),
                                     fmt(phase_loads[2]), fmt(circuit["voltageDrop"]), fmt(circuit["cableLength"]),
                                     circuit["noteText"]))

        count = len(self.boards)
        self.count_label.configure(text=str(count), foreground="#c62828" if flags else "#1b7f3b")
        self.summary_label.configure(text=f"distribution board{'s' if count != 1 else ''}"
                                          + (f", {flags} flag(s)" if flags else ""))
        status = self.read_status
        if status:
            self.window.set_status(*status)
        elif not count:
            self.window.set_status("No distribution boards found.", "warning")
        else:
            self.window.set_status(f"{flags} flag(s), shown in red." if flags else "Done, nothing flagged.",
                                   "warning" if flags else "ok")

    # Saving

    def export(self) -> None:
        if not self.boards:
            self.window.set_status("Press Refresh first.", "warning")
            return
        self._annotate()
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the schedule",
                                            initialfile=f"Distribution boards {stamp}.csv", defaultextension=".csv",
                                            filetypes=[FILE_TYPES[1], FILE_TYPES[0]])
        if not path:
            return

        def value(number_or_none):
            return "" if number_or_none is None else round(number_or_none, 3)

        rows = []
        for board in self.boards:
            for circuit in board["circuits"]:
                phase_loads = circuit["phaseLoads"] or [None, None, None]
                rows.append([board["name"], circuit["name"], circuit["description"], circuit["phases"],
                             value(circuit["voltage"]), value(circuit["load"]),
                             *[value(load) for load in phase_loads],
                             value(circuit["voltageDrop"]), value(circuit["cableLength"]), circuit["noteText"]])
            phase_loads = board["phaseLoads"] or [None, None, None]
            rows.append([board["name"], "Total", board["description"], "", "", value(board["total"]),
                         *[value(load) for load in phase_loads], "", "", board["noteText"]])
        write_table(path, EXPORT_HEADERS, rows, "Distribution boards")
        self.window.set_status(f"Saved {len(self.boards)} board(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run()


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    DistributionBoardSchedule().run()
