# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "documentation-layouts"
# archicad-versions = [27, 28, 29, 30]
# min-tapir-version = "1.8.1"
# changes = "model"
# ///

"""
Symbol Legend

Finds the objects and lamps placed on a story (or in the selection, or in the
whole project), optionally only those on chosen layers, and lists one row per
library part with the number of placed instances. Untick the rows to leave out,
then Create draws the legend in a new worksheet: one instance of each symbol in
a column, with the same size as the first placed one, its name (or the value of
a description parameter) beside it, and the count when asked for.

The worksheet can then be placed on a layout like any other drawing.
"""

import re
import tkinter as tk
from tkinter import simpledialog, ttk

from utilities.archicad import (ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                item_errors, run_tapir)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Symbol Legend"
TYPES = ["Object", "Lamp"]
CHECKED, UNCHECKED = "☑", "☐"
GDL_BATCH_SIZE = 200
CREATE_BATCH_SIZE = 500
SCOPE_CURRENT, SCOPE_SELECTION, SCOPE_PROJECT = "Current story", "Selected elements", "Whole project"
STORY_PREFIX = "Story: "

LENGTH_UNITS = {"": 1.0, "m": 1.0, "cm": 0.01, "mm": 0.001, "ft": 0.3048, "'": 0.3048, "in": 0.0254, '"': 0.0254}


def parse_length(text: str) -> float | None:
    """ A length in metres from text like 0.8, 0,8, 80 cm, 800mm, 2.5 ft, 2' or 30 in. """
    match = re.fullmatch(r"\s*([-+]?\d+(?:[.,]\d*)?|[-+]?[.,]\d+)\s*(m|cm|mm|ft|'|in|\")?\s*", text or "")
    if not match:
        return None
    return float(match.group(1).replace(",", ".")) * LENGTH_UNITS[match.group(2) or ""]


def parse_number(text: str) -> float | None:
    try:
        return float((text or "").strip().replace(",", "."))
    except ValueError:
        return None


class SymbolLegend:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Draws a legend of the objects and lamps placed on a story in a new worksheet: one symbol per "
            "library part with its name beside it. Press Find symbols, untick the ones to leave out, then Create.",
            min_width=760, min_height=680)
        self.stories: dict[int, str] = {}
        self.layers: dict[str, int] = {}
        self.layer_labels: list[str] = []
        # one per library part: {"type", "part", "label", "count", "include", "dimensions"}
        self.rows: list[dict] = []
        self.found_with: tuple | None = None
        self._build_body()
        self.window.add_button("Create", self.create)
        self.window.add_button("Find symbols", self.find, primary=True)

    # ---- window ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)

        source = ttk.LabelFrame(body, text="Symbols to list", padding=(10, 6))
        source.grid(row=0, column=0, sticky="ew")
        source.columnconfigure(1, weight=1)
        ttk.Label(source, text="Where:").grid(row=0, column=0, sticky="w")
        self.scope = tk.StringVar(value=SCOPE_CURRENT)
        self.scope_box = ttk.Combobox(source, textvariable=self.scope, state="readonly", width=40,
                                      values=[SCOPE_CURRENT, SCOPE_SELECTION, SCOPE_PROJECT])
        self.scope_box.grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Label(source, text="Layers (none selected: all layers):").grid(row=1, column=0, columnspan=2,
                                                                          sticky="w", pady=(6, 0))
        layer_frame = ttk.Frame(source)
        layer_frame.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(2, 0))
        layer_frame.columnconfigure(0, weight=1)
        self.layer_list = tk.Listbox(layer_frame, selectmode="extended", height=5, exportselection=False)
        self.layer_list.grid(row=0, column=0, sticky="ew")
        scrollbar = ttk.Scrollbar(layer_frame, orient="vertical", command=self.layer_list.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.layer_list.configure(yscrollcommand=scrollbar.set)
        ttk.Label(source, text="Description parameter (optional, e.g. a GDL parameter holding the "
                               "symbol's description; its name or its label):").grid(
            row=3, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.description_parameter = tk.StringVar()
        ttk.Entry(source, textvariable=self.description_parameter, width=40).grid(
            row=4, column=0, columnspan=2, sticky="w", pady=(2, 0))

        listed = ttk.Frame(body, padding=(0, 8, 0, 0))
        listed.grid(row=1, column=0, sticky="nsew")
        listed.columnconfigure(0, weight=1)
        listed.rowconfigure(1, weight=1)
        self.summary_label = ttk.Label(listed, text="Press Find symbols. Click a row's box (or press Space) to "
                                                    "leave it out of the legend; double-click edits its text.")
        self.summary_label.grid(row=0, column=0, sticky="w", pady=(0, 4))
        self.tree = make_tree(listed, [("include", "In legend", 70, "center"), ("label", "Text in the legend", 320, "w"),
                                       ("part", "Library part", 220, "w"), ("type", "Type", 60, "w"),
                                       ("count", "Count", 60, "e")], height=10)
        self.tree.container.grid(row=1, column=0, sticky="nsew")
        self.tree.bind("<Button-1>", self._on_click)
        self.tree.bind("<space>", lambda _e: self._toggle(self.tree.selection()))
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self._edit_label)())

        legend = ttk.LabelFrame(body, text="Legend", padding=(10, 6))
        legend.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        self.fields: dict[str, tk.StringVar] = {}
        for row, (key, label, default) in enumerate([
                ("name", "Worksheet name:", "Symbol legend"),
                ("reference", "Worksheet reference ID:", "LEG"),
                ("title", "Title (empty: none):", "Legend"),
                ("spacing", "Row spacing (smallest; m, cm, mm, ft or in):", "0.8"),
                ("gap", "Gap between symbol and text:", "0.5"),
                ("height", "Text height (mm, on paper):", "2.5")]):
            ttk.Label(legend, text=label).grid(row=row, column=0, sticky="w", pady=(2, 0))
            var = tk.StringVar(value=default)
            ttk.Entry(legend, textvariable=var, width=24).grid(row=row, column=1, sticky="w", padx=(6, 0), pady=(2, 0))
            self.fields[key] = var
        self.show_count = tk.BooleanVar(value=False)
        ttk.Checkbutton(legend, text="Show the count beside each name", variable=self.show_count).grid(
            row=6, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.open_worksheet = tk.BooleanVar(value=True)
        ttk.Checkbutton(legend, text="Open the worksheet afterwards", variable=self.open_worksheet).grid(
            row=7, column=0, columnspan=2, sticky="w")

    def _on_click(self, event) -> None:
        if self.tree.identify_region(event.x, event.y) == "cell" and self.tree.identify_column(event.x) == "#1":
            row = self.tree.identify_row(event.y)
            if row:
                self._toggle([row])

    def _toggle(self, rows) -> None:
        for row in rows:
            item = self.rows[int(row)]
            item["include"] = not item["include"]
            self.tree.set(row, "include", CHECKED if item["include"] else UNCHECKED)
        self._show_summary()

    def _edit_label(self) -> None:
        row = self.tree.focus()
        if not row:
            return
        item = self.rows[int(row)]
        text = simpledialog.askstring(SCRIPT_NAME, f"The text beside {item['part']} in the legend:",
                                      initialvalue=item["label"], parent=self.window.root)
        if text is not None and text.strip():
            item["label"] = text.strip()
            self.tree.set(row, "label", item["label"])

    def _show_summary(self) -> None:
        included = [r for r in self.rows if r["include"]]
        self.summary_label.configure(
            text=f"{len(self.rows)} symbol(s) found, {len(included)} in the legend "
                 f"({sum(r['count'] for r in included)} placed instance(s)). Click a box to leave a symbol out; "
                 "double-click edits its text.")

    # ---- start ----

    def load(self) -> None:
        with self.window.busy("Reading the stories and the layers..."):
            self.stories = get_story_names()
            layers = run_tapir("GetAttributesByType", {"attributeType": "Layer"}).get("attributes", [])
        self.scope_box.configure(values=[SCOPE_CURRENT, SCOPE_SELECTION, SCOPE_PROJECT] +
                                 [f"{STORY_PREFIX}{name}" for _, name in sorted(self.stories.items())])
        self.layers = {}
        for layer in sorted(layers, key=lambda a: str(a.get("name", "")).lower()):
            label = str(layer.get("name", ""))
            if label in self.layers:
                label = f"{label} ({layer['index']})"
            self.layers[label] = int(layer["index"])
        self.layer_labels = list(self.layers)
        self.layer_list.delete(0, "end")
        for label in self.layer_labels:
            self.layer_list.insert("end", label)
        self.window.set_status("Pick where to look and press Find symbols.")

    # ---- finding ----

    def _settings_key(self) -> tuple:
        return (self.scope.get(), tuple(self.layer_list.curselection()), self.description_parameter.get().strip())

    def _story_of_scope(self) -> int | None:
        scope = self.scope.get()
        if not scope.startswith(STORY_PREFIX):
            return None
        name = scope[len(STORY_PREFIX):]
        return next((index for index, story in self.stories.items() if story == name), None)

    def find(self) -> None:
        scope = self.scope.get()
        story = self._story_of_scope()
        wanted_layers = {self.layers[self.layer_labels[i]] for i in self.layer_list.curselection()}
        with self.window.busy("Reading the placed objects and lamps..."):
            mode = {SCOPE_CURRENT: "story", SCOPE_SELECTION: "selection"}.get(scope, "project")
            elements = get_elements(mode, TYPES)
            details = get_details_of_elements(elements, ["type", "floorIndex", "layerIndex", "details"])
            groups: dict[tuple, dict] = {}
            for element, detail in zip(elements, details):
                if "error" in detail or detail.get("type") not in TYPES:
                    continue
                if story is not None and detail.get("floorIndex") != story:
                    continue
                if wanted_layers and detail.get("layerIndex") not in wanted_layers:
                    continue
                specific = detail.get("details") or {}
                part = (specific.get("libPart") or {}).get("name")
                if not part:
                    continue
                key = (detail["type"], part)
                group = groups.get(key)
                if group is None:
                    groups[key] = {"type": detail["type"], "part": part, "label": part, "count": 1, "include": True,
                                   "first": element, "dimensions": specific.get("dimensions")}
                else:
                    group["count"] += 1

            rows = sorted(groups.values(), key=lambda g: (g["type"], g["part"].lower()))
            note = ""
            parameter = self.description_parameter.get().strip()
            if parameter and rows:
                self.window.set_status(f"Reading the parameter {parameter}...")
                found = self._read_descriptions(rows, parameter)
                if not found:
                    note = f" None of the symbols has a text parameter {parameter}; the library part names are used."

        self.rows = rows
        self.found_with = self._settings_key()
        self.tree.delete(*self.tree.get_children())
        for index, row in enumerate(rows):
            self.tree.insert("", "end", iid=str(index), values=(CHECKED, row["label"], row["part"], row["type"],
                                                                row["count"]))
        self._show_summary()
        if not rows:
            where = "in the selection" if scope == SCOPE_SELECTION else "there"
            self.window.set_status(f"No objects or lamps {where}{' on the chosen layers' if wanted_layers else ''}.",
                                   "warning")
            return
        self.window.set_status(f"Found {len(rows)} symbol(s). Untick the ones to leave out, then press Create.{note}",
                               "warning" if note else "ok")

    def _read_descriptions(self, rows: list[dict], parameter: str) -> int:
        """ Takes the text of the parameter (by name or label) of each symbol's first instance as its
        label; returns how many had a non-empty one. """
        wanted = parameter.lower()
        found = 0
        for chunk in batched(rows, GDL_BATCH_SIZE):
            result = run_tapir("GetGDLParametersOfElements", {
                "elements": [{"elementId": r["first"]["elementId"]} for r in chunk]}).get("gdlParametersOfElements", [])
            for row, entry in zip(chunk, result):
                if not isinstance(entry, dict) or "error" in entry:
                    continue
                for p in entry.get("parameters", []):
                    if p.get("name", "").lower() == wanted or (p.get("displayName") or "").lower() == wanted:
                        value = p.get("value")
                        if isinstance(value, str) and value.strip():
                            row["label"] = value.strip()
                            found += 1
                        break
        return found

    # ---- creating ----

    def _legend_settings(self) -> dict | None:
        settings = {key: var.get().strip() for key, var in self.fields.items()}
        if not settings["name"] or not settings["reference"]:
            self.window.set_status("The worksheet needs a name and a reference ID.", "warning")
            return None
        for key, label in [("spacing", "Row spacing"), ("gap", "The gap")]:
            value = parse_length(settings[key])
            if value is None or value <= 0:
                self.window.set_status(f"{label} must be a positive length, e.g. 0.8 or 80 cm.", "warning")
                return None
            settings[key] = value
        height = parse_number(settings["height"])
        if height is None or height <= 0:
            self.window.set_status("The text height must be a positive number of millimetres.", "warning")
            return None
        settings["height"] = height
        return settings

    def _layout(self, rows: list[dict], settings: dict) -> tuple[list[dict], list[dict]]:
        """ The symbols in a column from the origin downwards, the texts beside them. """
        sizes = []
        for row in rows:
            dims = row.get("dimensions") or {}
            sizes.append((abs(float(dims.get("x") or 0.0)), abs(float(dims.get("y") or 0.0))))
        text_x = max((w for w, _ in sizes), default=0.0) + settings["gap"]
        symbols, texts = [], []
        y = 0.0
        if settings["title"]:
            texts.append({"text": settings["title"], "coordinate": {"x": 0.0, "y": settings["spacing"], "z": 0.0},
                          "height": settings["height"] * 1.4, "justification": "Left"})
        for row, (_, depth) in zip(rows, sizes):
            pitch = max(settings["spacing"], depth + settings["gap"])
            symbol = {"libraryPartName": row["part"], "coordinates": {"x": 0.0, "y": y, "z": 0.0}, "angle": 0.0}
            dims = row.get("dimensions")
            if isinstance(dims, dict) and all(k in dims for k in ("x", "y", "z")):
                symbol["dimensions"] = {"x": dims["x"], "y": dims["y"], "z": dims["z"]}
            symbols.append({"type": row["type"], "item": symbol})
            label = f"{row['label']} ({row['count']})" if self.show_count.get() else row["label"]
            texts.append({"text": label, "coordinate": {"x": text_x, "y": y, "z": 0.0},
                          "height": settings["height"], "justification": "Left"})
            y -= pitch
        return symbols, texts

    def create(self) -> None:
        if not self.rows:
            self.window.set_status("Press Find symbols first.", "warning")
            return
        if self.found_with != self._settings_key():
            self.find()
            self.window.set_status("The list is updated to the current settings. Check it, then press Create again.",
                                   "warning")
            return
        rows = [r for r in self.rows if r["include"]]
        if not rows:
            self.window.set_status("Every symbol is unticked; tick at least one.", "warning")
            return
        settings = self._legend_settings()
        if settings is None:
            return
        names = "\n".join(f"  {r['label']}" + (f" ({r['count']})" if self.show_count.get() else "") for r in rows[:15])
        if len(rows) > 15:
            names += f"\n  ... and {len(rows) - 15} more"
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Create the worksheet '{settings['name']}' "
                                                      f"({settings['reference']}) with {len(rows)} symbol(s)?\n\n{names}"):
            return

        symbols, texts = self._layout(rows, settings)
        errors: list[str] = []
        created = 0
        with self.window.busy("Creating the worksheet..."):
            result = run_tapir("CreateWorksheets", {"worksheetsData": [
                {"name": settings["name"], "referenceId": settings["reference"]}]}).get("databases", [])
            worksheet = result[0] if result else {}
            if "databaseId" not in worksheet:
                message = item_errors(result)[0] if item_errors(result) else "no database was returned"
                raise ArchicadError(f"The worksheet could not be created: {message}")
            database_id = worksheet["databaseId"]

            for type_id, command, key in [("Object", "CreateObjects", "objectsData"), ("Lamp", "CreateLamps", "lampsData")]:
                items = [s["item"] for s in symbols if s["type"] == type_id]
                for chunk in batched(items, CREATE_BATCH_SIZE):
                    self.window.set_status(f"Placing the symbols... {created} / {len(symbols)}")
                    placed = run_tapir(command, {"databaseId": database_id, key: chunk}).get("elements", [])
                    created += sum(1 for e in placed if isinstance(e, dict) and "elementId" in e)
                    errors += item_errors(placed)

            self.window.set_status("Writing the texts...")
            for chunk in batched(texts, CREATE_BATCH_SIZE):
                written = run_tapir("CreateTexts", {"databaseId": database_id, "textsData": chunk}).get("elements", [])
                errors += item_errors(written)

            if self.open_worksheet.get():
                try:
                    run_tapir("ChangeWindow", {"databaseId": database_id})
                except ArchicadError as e:
                    print(f"The worksheet could not be opened: {e}")

        for error in errors[:20]:
            print(f"Error: {error}")
        if errors:
            self.window.set_status(f"The legend is in '{settings['name']}' with {created} of {len(symbols)} symbol(s); "
                                   f"{len(errors)} error(s), the first: {errors[0]}",
                                   "error" if not created else "warning")
        else:
            self.window.set_status(f"The legend with {created} symbol(s) is in the worksheet '{settings['name']}'. "
                                   "Place it on a layout from the Project Map.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    SymbolLegend().run()
