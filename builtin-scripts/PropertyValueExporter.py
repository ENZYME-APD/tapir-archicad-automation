# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Property Value Exporter

Writes the values of the chosen properties of the chosen element types into an
Excel workbook or a CSV file, one row per element, without changing anything
in the project. It replaces building an interactive schedule only to copy its
values into Excel.

The file has a header row and one row per element. The columns are:
  GUID        the element's GUID, which identifies the element when the file is read back
  Element ID  the Element ID shown in Archicad
  Type        the element type (Wall, Slab, Zone, ...)
  Story       the name of the element's home story
  then one column per chosen property, headed "<property group>: <property name>"
  (the names GetAllProperties gives), holding the value as Archicad displays it,
  or with "Precise numbers" the number in SI units (m, m², m³, degrees).
A property the element does not have, or has no value for, gives an empty cell.
The values are text in the file, so Excel keeps them exactly as Archicad gave them.
"""

import datetime
import os
import re
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, get_story_names, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Property Value Exporter"
FIXED_HEADERS = ["GUID", "Element ID", "Type", "Story"]
ELEMENT_TYPES = [
    "Wall", "Column", "Beam", "Slab", "Roof", "Shell", "Morph", "Mesh", "Window", "Door", "Skylight",
    "Opening", "Object", "Lamp", "Zone", "CurtainWall", "Stair", "Railing",
]


def property_header(prop: dict) -> str:
    return f"{prop.get('propertyGroupName', '')}: {prop.get('propertyName', '')}"


def natural_key(text: str):
    """ Sorts "Room 2" before "Room 10". """
    return [(0, float(part), "") if part.isdigit() else (1, 0.0, part.lower()) for part in re.split(r"(\d+)", text)]


class PropertyValueExporter:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Writes the values of the chosen properties into Excel or CSV, one row per element, "
            "with its Element ID, type and story. Choose the element types and the properties, "
            "then press Export. Nothing in the project is changed.",
            min_width=760, min_height=640)
        self.properties: list[dict] = []
        self.listed_properties: list[dict] = []
        self.chosen_properties: list[dict] = []
        self._build_body()
        self.window.add_button("Export...", self.export, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)

        elements = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        elements.grid(row=0, column=0, sticky="ew")
        elements.columnconfigure(5, weight=1)
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(elements, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))
        self.visible_only = tk.BooleanVar(value=False)
        ttk.Checkbutton(elements, text="Only elements on visible layers", variable=self.visible_only).grid(
            row=0, column=3, sticky="w", padx=(18, 0))
        ttk.Label(elements, text="Types:").grid(row=1, column=0, sticky="nw", pady=(8, 0))
        types = ttk.Frame(elements)
        types.grid(row=2, column=0, columnspan=6, sticky="ew", pady=(2, 0))
        self.type_vars = {t: tk.BooleanVar(value=False) for t in ELEMENT_TYPES}
        per_row = 6
        for index, element_type in enumerate(ELEMENT_TYPES):
            ttk.Checkbutton(types, text=element_type, variable=self.type_vars[element_type]).grid(
                row=index // per_row, column=index % per_row, sticky="w", padx=(0, 12))
        type_buttons = ttk.Frame(elements)
        type_buttons.grid(row=3, column=0, columnspan=6, sticky="w", pady=(4, 0))
        ttk.Button(type_buttons, text="All types", command=lambda: self._set_types(True)).pack(side="left")
        ttk.Button(type_buttons, text="No types", command=lambda: self._set_types(False)).pack(side="left", padx=(6, 0))

        properties = ttk.LabelFrame(body, text="Properties", padding=(10, 6))
        properties.grid(row=1, column=0, sticky="nsew", pady=(8, 0))
        properties.columnconfigure(0, weight=1)
        properties.columnconfigure(2, weight=1)
        properties.rowconfigure(2, weight=1)

        search = ttk.Frame(properties)
        search.grid(row=0, column=0, sticky="ew")
        search.columnconfigure(1, weight=1)
        ttk.Label(search, text="Search:").grid(row=0, column=0, sticky="w")
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_: self._list_properties())
        ttk.Entry(search, textvariable=self.search).grid(row=0, column=1, sticky="ew", padx=(6, 0))
        ttk.Label(properties, text="Available").grid(row=1, column=0, sticky="w", pady=(6, 0))
        ttk.Label(properties, text="Exported, in this column order").grid(row=1, column=2, sticky="w", pady=(6, 0))

        self.property_tree = make_tree(properties, [("name", "Property", 200, "w"), ("group", "Group", 150, "w")],
                                       height=10)
        self.property_tree.container.grid(row=2, column=0, sticky="nsew", pady=(2, 0))
        self.property_tree.bind("<Double-1>", lambda _e: self.window.guarded(self.add_properties)())

        moves = ttk.Frame(properties, padding=(8, 0))
        moves.grid(row=2, column=1, sticky="ns")
        ttk.Button(moves, text="Add →", command=self.window.guarded(self.add_properties)).pack(fill="x", pady=(30, 4))
        ttk.Button(moves, text="← Remove", command=self.window.guarded(self.remove_properties)).pack(fill="x", pady=4)
        ttk.Button(moves, text="Up", command=self.window.guarded(lambda: self.move_properties(-1))).pack(fill="x", pady=(16, 4))
        ttk.Button(moves, text="Down", command=self.window.guarded(lambda: self.move_properties(1))).pack(fill="x", pady=4)

        self.chosen_tree = make_tree(properties, [("name", "Property", 200, "w"), ("group", "Group", 150, "w")],
                                     height=10)
        self.chosen_tree.container.grid(row=2, column=2, sticky="nsew", pady=(2, 0))
        self.chosen_tree.bind("<Double-1>", lambda _e: self.window.guarded(self.remove_properties)())

        options = ttk.Frame(body, padding=(0, 8, 0, 0))
        options.grid(row=2, column=0, sticky="ew")
        self.si_units = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Precise numbers (SI units: m, m², m³, degrees) instead of the displayed values",
                        variable=self.si_units).grid(row=0, column=0, sticky="w")

    def _set_types(self, value: bool) -> None:
        for variable in self.type_vars.values():
            variable.set(value)

    # ---- properties ----

    def load_properties(self) -> None:
        with self.window.busy("Reading the properties..."):
            properties = run_tapir("GetAllProperties").get("properties", [])
        self.properties = sorted(
            (p for p in properties if "propertyId" in p),
            key=lambda p: (p.get("propertyType") != "Custom", p.get("propertyGroupName", "").lower(),
                           p.get("propertyName", "").lower()))
        self._list_properties()
        self.window.set_status(f"{len(self.properties)} properties. Choose the types and the properties, then press Export.")

    def _list_properties(self) -> None:
        words = self.search.get().lower().split()
        chosen = {p["propertyId"]["guid"] for p in self.chosen_properties}
        self.listed_properties = [
            p for p in self.properties
            if p["propertyId"]["guid"] not in chosen
            and all(w in f"{p.get('propertyGroupName', '')} {p.get('propertyName', '')}".lower() for w in words)]
        self.property_tree.delete(*self.property_tree.get_children())
        for index, p in enumerate(self.listed_properties):
            self.property_tree.insert("", "end", iid=str(index), values=(p.get("propertyName", ""), p.get("propertyGroupName", "")))

    def _show_chosen(self, selected: list[int] | None = None) -> None:
        self.chosen_tree.delete(*self.chosen_tree.get_children())
        for index, p in enumerate(self.chosen_properties):
            self.chosen_tree.insert("", "end", iid=str(index), values=(p.get("propertyName", ""), p.get("propertyGroupName", "")))
        if selected:
            self.chosen_tree.selection_set([str(i) for i in selected])
            self.chosen_tree.see(str(selected[0]))

    def add_properties(self) -> None:
        rows = self.property_tree.selection()
        if not rows:
            self.window.set_status("Select properties in the Available list first.", "warning")
            return
        self.chosen_properties += [self.listed_properties[int(r)] for r in rows]
        self._list_properties()
        self._show_chosen()
        self.window.set_status(f"{len(self.chosen_properties)} propert{'y' if len(self.chosen_properties) == 1 else 'ies'} chosen.")

    def remove_properties(self) -> None:
        rows = {int(r) for r in self.chosen_tree.selection()}
        if not rows:
            return
        self.chosen_properties = [p for i, p in enumerate(self.chosen_properties) if i not in rows]
        self._list_properties()
        self._show_chosen()

    def move_properties(self, step: int) -> None:
        rows = sorted(int(r) for r in self.chosen_tree.selection())
        if not rows:
            return
        if (step < 0 and rows[0] == 0) or (step > 0 and rows[-1] == len(self.chosen_properties) - 1):
            return
        for index in (rows if step < 0 else reversed(rows)):
            items = self.chosen_properties
            items[index], items[index + step] = items[index + step], items[index]
        self._show_chosen([i + step for i in rows])

    # ---- export ----

    def _read_values(self, elements: list[dict], property_ids: list[dict]) -> tuple[list[list[str]], int]:
        """ One list of value texts per element, in the order of the properties, and the number
        of elements whose values could not be read (their cells stay empty). """
        failed = 0
        use_si = self.si_units.get()
        values: list[list[str]] = []
        for chunk in batched(elements):
            self.window.set_status(f"Reading the values... {len(values)} / {len(elements)}")
            parameters = {"elements": chunk, "properties": property_ids}
            if use_si:
                try:
                    result = run_tapir("GetPropertyValuesOfElements", {**parameters, "useSIUnits": True})
                except ArchicadError:
                    # Add-Ons before useSIUnits reject the parameter; they give the displayed values
                    use_si = False
                    self.si_units.set(False)
                    print("This Tapir Add-On cannot give precise numbers; the displayed values are exported.")
                    result = run_tapir("GetPropertyValuesOfElements", parameters)
            else:
                result = run_tapir("GetPropertyValuesOfElements", parameters)
            for entry in result.get("propertyValuesForElements", []):
                if "error" in entry:
                    failed += 1
                items = entry.get("propertyValues") or []
                row = [item["propertyValue"].get("value", "") if "propertyValue" in item else "" for item in items]
                values.append(row + [""] * (len(property_ids) - len(row)))
        return values, failed

    def export(self) -> None:
        element_types = [t for t in ELEMENT_TYPES if self.type_vars[t].get()]
        if not element_types:
            self.window.set_status("Tick at least one element type.", "warning")
            return
        if not self.chosen_properties:
            self.window.set_status("Add at least one property to the Exported list.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the property values",
                                            initialfile=f"Property values {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return

        properties = list(self.chosen_properties)
        filters = ["IsVisibleByLayer"] if self.visible_only.get() else []
        with self.window.busy("Reading the elements..."):
            elements = get_elements(self.scope.get(), element_types, filters)
            if not elements:
                self.window.set_status("There are no elements of the chosen types here.", "warning")
                return
            details = get_details_of_elements(elements, ["type", "id", "floorIndex"])
            stories = get_story_names()
            values, failed = self._read_values(elements, [{"propertyId": p["propertyId"]} for p in properties])

            # sorted by type, then story from the bottom up, then Element ID
            order = {t: i for i, t in enumerate(ELEMENT_TYPES)}
            keyed_rows = []
            for element, detail, element_values in zip(elements, details, values):
                element_type = detail.get("type", "")
                element_id = detail.get("id", "")
                floor = detail.get("floorIndex")
                story = stories.get(floor, "" if floor is None else str(floor))
                key = (order.get(element_type, len(order)), element_type,
                       floor if isinstance(floor, (int, float)) else 0, natural_key(element_id))
                keyed_rows.append((key, [element["elementId"]["guid"], element_id, element_type, story, *element_values]))
            keyed_rows.sort(key=lambda kr: kr[0])
            rows = [row for _, row in keyed_rows]
            self.window.set_status("Writing the file...")
            try:
                write_table(path, FIXED_HEADERS + [property_header(p) for p in properties], rows, "Property values")
            except OSError as e:
                self.window.set_status(f"Cannot write {os.path.basename(path)} (is it open in Excel?): {e}", "error")
                return

        note = f" The values of {failed} element(s) could not be read; their cells are empty." if failed else ""
        self.window.set_status(f"Exported {len(rows)} element(s) with {len(properties)} propert"
                               f"{'y' if len(properties) == 1 else 'ies'} to {os.path.basename(path)}.{note}",
                               "warning" if failed else "ok")

    def run(self) -> None:
        self.window.run(on_start=self.load_properties)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    PropertyValueExporter().run()
