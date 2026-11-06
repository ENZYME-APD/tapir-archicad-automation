# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Material Take-Off

Totals the volume (m³) and the projected area (m²) of every building material
used by the walls, columns, beams, slabs, roofs and shells, per story and
optionally per element type, from the quantities Archicad calculates for each
skin. The table can be copied to the clipboard or saved as Excel or CSV for a
bill of quantities. It only reads the project.
"""

import datetime
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, batched, get_details_of_elements, get_elements, get_story_names, \
    item_errors, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Material Take-Off"
# The types GetQuantitiesOfElements reports building materials for. Morphs and Zones have no skins
# there, and the segments of columns and beams are already part of their column or beam.
ELEMENT_TYPES = ["Wall", "Column", "Beam", "Slab", "Roof", "Shell"]
QUANTITIES_BATCH_SIZE = 200
VOLUME_DIGITS = 3
AREA_DIGITS = 2
NULL_GUID = "00000000-0000-0000-0000-000000000000"


class MaterialTakeOffWindow:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Totals the volume and the projected area of each building material of the walls, columns, beams, "
            "slabs, roofs and shells, per story, from the quantities Archicad calculates for each skin. "
            "Nothing in the project is changed.",
            min_width=720, min_height=520)
        self.story_names: dict[int, str] = {}
        self.rows: list[list] = []
        self.headers: list[str] = []
        self._build_body()
        self.window.add_button("Copy to clipboard", self.copy)
        self.window.add_button("Save as...", self.save)
        self.window.add_button("Calculate", self.calculate, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Settings", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(4, weight=1)

        self.scope = tk.StringVar(value="project")
        ttk.Label(options, text="Elements:").grid(row=0, column=0, sticky="w")
        for column, (value, text) in enumerate((("project", "Whole project"), ("story", "Current story"),
                                                ("selection", "Selection")), start=1):
            ttk.Radiobutton(options, text=text, value=value, variable=self.scope).grid(
                row=0, column=column, sticky="w", padx=(6 if column == 1 else 10, 0))

        self.by_type = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Group by element type as well", variable=self.by_type).grid(
            row=1, column=0, columnspan=4, sticky="w", pady=(6, 0))
        self.visible_only = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Only elements on visible layers", variable=self.visible_only).grid(
            row=2, column=0, columnspan=4, sticky="w", pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("materials", "building materials"), ("volume", "m³ in total"),
                                                 ("area", "m² projected area"), ("elements", "elements counted")]):
            summary.columnconfigure(column, weight=1)
            value = ttk.Label(summary, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(summary, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        self.tree_frame = ttk.Frame(body)
        self.tree_frame.grid(row=2, column=0, sticky="nsew")
        self.tree_frame.columnconfigure(0, weight=1)
        self.tree_frame.rowconfigure(0, weight=1)
        self._make_tree(by_type=False)

    def _make_tree(self, by_type: bool) -> None:
        for child in self.tree_frame.winfo_children():
            child.destroy()
        columns = [("story", "Story", 140, "w")]
        if by_type:
            columns.append(("type", "Element type", 100, "w"))
        columns += [("material", "Building material", 220, "w"), ("elements", "Elements", 75, "e"),
                    ("volume", "Volume (m³)", 100, "e"), ("area", "Projected area (m²)", 140, "e")]
        self.tree = make_tree(self.tree_frame, columns, height=14)
        self.tree.container.grid(row=0, column=0, sticky="nsew")

    # --- reading -----------------------------------------------------------

    def calculate(self) -> None:
        by_type = self.by_type.get()
        warnings: list[str] = []
        with self.window.busy("Reading the elements..."):
            self.story_names = get_story_names()
            filters = ["IsVisibleByLayer"] if self.visible_only.get() else []
            elements = get_elements(self.scope.get(), ELEMENT_TYPES, filters)
            details = get_details_of_elements(elements, ["type", "floorIndex"])
            warnings += item_errors(details)

            # (floor index, element type or "", building material guid) -> [volume, area, element guids]
            totals: dict[tuple[int, str, str], list] = {}
            counted: set[str] = set()
            done = 0
            for chunk_elements, chunk_details in zip(batched(elements, QUANTITIES_BATCH_SIZE),
                                                     batched(details, QUANTITIES_BATCH_SIZE)):
                self.window.set_status(f"Reading the quantities... {done}/{len(elements)}")
                self.window.root.update()
                done += len(chunk_elements)
                quantities = run_tapir("GetQuantitiesOfElements", {"elements": chunk_elements}).get(
                    "quantitiesOfElements", [])
                warnings += item_errors(quantities)
                for element, detail, quantity in zip(chunk_elements, chunk_details, quantities):
                    if "error" in detail or "error" in quantity:
                        continue
                    guid = element["elementId"]["guid"]
                    floor = detail.get("floorIndex", 0)
                    element_type = (detail.get("type") or quantity.get("type", "")) if by_type else ""
                    for skin in quantity.get("composites", []):
                        material = (skin.get("buildingMaterialId") or {}).get("attributeId", {}).get("guid", "")
                        if material == NULL_GUID:
                            material = ""
                        entry = totals.setdefault((floor, element_type, material), [0.0, 0.0, set()])
                        entry[0] += skin.get("volume", 0.0)
                        entry[1] += skin.get("projectedArea", 0.0)
                        entry[2].add(guid)
                        counted.add(guid)

            self.window.set_status("Reading the building materials...")
            names = self._material_names(sorted({key[2] for key in totals if key[2]}), warnings)

        self._fill(totals, names, by_type, len(counted))
        if not elements:
            self.window.set_status("There are no walls, columns, beams, slabs, roofs or shells to count.", "warning")
        elif warnings:
            for warning in warnings:
                print(f"Problem: {warning}")
            self.window.set_status(f"Done, with {len(warnings)} problem(s): {warnings[0]}", "warning")
        elif totals and not any(entry[1] for entry in totals.values()):
            self.window.set_status("Done. Every projected area is 0: Archicad reports none when the legacy "
                                   "intersection and surface calculation is on in the Project Preferences.", "warning")
        else:
            self.window.set_status("Done.", "ok")

    def _material_names(self, guids: list[str], warnings: list[str]) -> dict[str, str]:
        names: dict[str, str] = {}
        for chunk in batched(guids):
            try:
                materials = run_tapir("GetBuildingMaterials", {
                    "attributeIds": [{"attributeId": {"guid": guid}} for guid in chunk]}).get("buildingMaterials", [])
            except ArchicadError as e:
                warnings.append(str(e))
                continue
            warnings += item_errors(materials)
            for guid, material in zip(chunk, materials):
                if "error" not in material:
                    names[guid] = material.get("name") or f"Building material {material.get('index', '')}".strip()
        return names

    def _fill(self, totals: dict[tuple[int, str, str], list], names: dict[str, str], by_type: bool,
              element_count: int) -> None:
        def material_name(guid: str) -> str:
            return names.get(guid) or ("(no building material)" if not guid else f"(unknown building material {guid})")

        keys = sorted(totals, key=lambda k: (-k[0], k[1], material_name(k[2]).lower()))
        self.headers = ["Story"] + (["Element type"] if by_type else []) + \
                       ["Building material", "Elements", "Volume (m³)", "Projected area (m²)"]
        self.rows = []
        for floor, element_type, material in keys:
            volume, area, guids = totals[(floor, element_type, material)]
            self.rows.append([self.story_names.get(floor, f"Story {floor}")] + ([element_type] if by_type else []) +
                             [material_name(material), len(guids), round(volume, VOLUME_DIGITS), round(area, AREA_DIGITS)])

        self._make_tree(by_type)
        for row in self.rows:
            self.tree.insert("", "end", values=row[:-2] + [f"{row[-2]:.{VOLUME_DIGITS}f}", f"{row[-1]:.{AREA_DIGITS}f}"])

        self.stat_labels["materials"].configure(text=str(len({k[2] for k in totals})))
        self.stat_labels["volume"].configure(text=f"{sum(e[0] for e in totals.values()):.{VOLUME_DIGITS}f}")
        self.stat_labels["area"].configure(text=f"{sum(e[1] for e in totals.values()):.{AREA_DIGITS}f}")
        self.stat_labels["elements"].configure(text=str(element_count))

    # --- output ------------------------------------------------------------

    def copy(self) -> None:
        if not self.rows:
            self.window.set_status("Press Calculate first.", "warning")
            return
        lines = ["\t".join(self.headers)] + ["\t".join(str(value) for value in row) for row in self.rows]
        self.window.root.clipboard_clear()
        self.window.root.clipboard_append("\n".join(lines) + "\n")
        self.window.set_status(f"Copied {len(self.rows)} row(s) to the clipboard.", "ok")

    def save(self) -> None:
        if not self.rows:
            self.window.set_status("Press Calculate first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the material take-off",
                                            initialfile=f"Material take-off {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        write_table(path, self.headers, self.rows, "Material take-off")
        self.window.set_status(f"Saved {len(self.rows)} row(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.calculate)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    MaterialTakeOffWindow().run()
