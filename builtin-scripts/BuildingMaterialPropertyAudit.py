# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "classification-attributes"
# changes = "model"
# ///

"""
Building Material Property Audit

Lists the value of a custom property for every Building Material of the
project, so the materials with an empty value stand out. Export the list to
Excel or CSV, or give the chosen materials a value: the script shows which
materials it will change and asks before changing them, then reports the
materials it could not change. Based on the
audit_building_material_properties.py workflow example.
"""

import datetime
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, batched, item_errors, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Building Material Property Audit"
NOT_AVAILABLE = "(not available)"
EMPTY = "(empty)"
PREVIEW_LINES = 15


class BuildingMaterialPropertyAudit:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Shows the value of a custom property for every Building Material. Pick a property; select materials "
            "and press Set value to give them a value. With nothing selected, the listed materials with an empty "
            "value get it.",
            min_width=640, min_height=560)
        self.materials: list[dict] = []
        self.properties: list[dict] = []
        self.listed: list[dict] = []
        self._build_body()
        self.window.add_button("Export...", self.export)
        self.window.add_button("Set value...", self.set_value)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Property", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(0, weight=1)
        self.property = tk.StringVar()
        self.property_box = ttk.Combobox(options, textvariable=self.property, state="readonly")
        self.property_box.grid(row=0, column=0, sticky="ew")
        self.property_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.read_values)())
        self.only_empty = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Only materials with an empty value", variable=self.only_empty,
                        command=self._fill_tree).grid(row=1, column=0, sticky="w", pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Pick a property.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("name", "Building Material", 220, "w"),
            ("folder", "Folder", 140, "w"),
            ("value", "Value", 180, "w"),
        ], height=12)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("empty", foreground="#c62828")
        self.tree.tag_configure("unavailable", foreground="#777777")

        setter = ttk.Frame(body, padding=(0, 8, 0, 0))
        setter.grid(row=3, column=0, sticky="ew")
        setter.columnconfigure(1, weight=1)
        ttk.Label(setter, text="New value:").grid(row=0, column=0, sticky="w")
        self.new_value = tk.StringVar()
        ttk.Entry(setter, textvariable=self.new_value).grid(row=0, column=1, sticky="ew", padx=(6, 0))

    # ---- reading ----

    def refresh(self) -> None:
        with self.window.busy("Reading the Building Materials..."):
            attributes = run_tapir("GetAttributesByType", {"attributeType": "BuildingMaterial"}).get("attributes", [])
            self.materials = [{"attributeId": a["attributeId"], "name": a.get("name", ""),
                               "folder": " / ".join(a.get("folderPath", [])), "value": None,
                               "available": False, "editable": False} for a in attributes]
            property_guids = set()
            if self.materials:
                self.window.set_status("Reading the properties of the Building Materials...")
                for chunk in batched(self.materials):
                    definitions = run_tapir("GetPropertyDefinitionsOfAttributes", {
                        "attributeIds": [{"attributeId": m["attributeId"]} for m in chunk],
                        "filter": "UserDefined"}).get("propertyDefinitionsOfAttributes", [])
                    for definition in definitions:
                        property_guids.update(p["propertyId"]["guid"] for p in definition.get("propertyIds", []))
                all_properties = run_tapir("GetAllProperties").get("properties", [])
            else:
                all_properties = []
        self.properties = sorted(
            (p for p in all_properties if "propertyId" in p and p["propertyId"]["guid"] in property_guids),
            key=lambda p: (p.get("propertyGroupName", "").lower(), p.get("propertyName", "").lower()))

        previous = self.property.get()
        names = [self._property_label(p) for p in self.properties]
        self.property_box.configure(values=names)
        if not self.materials:
            self.window.set_status("The project has no Building Materials.", "warning")
            return
        if not names:
            self.property.set("")
            self.tree.delete(*self.tree.get_children())
            self.window.set_status("No custom property is available for the Building Materials. "
                                   "Make one available for them in the Property Manager.", "warning")
            return
        self.property_box.current(names.index(previous) if previous in names else 0)
        self.read_values()

    def _property_label(self, prop: dict) -> str:
        return f"{prop.get('propertyGroupName', '')} / {prop.get('propertyName', '')}"

    def _chosen_property(self) -> dict | None:
        index = self.property_box.current()
        return self.properties[index] if 0 <= index < len(self.properties) else None

    def read_values(self) -> None:
        prop = self._chosen_property()
        if prop is None:
            self.window.set_status("Pick a property first.", "warning")
            return
        unreadable = 0
        with self.window.busy("Reading the values..."):
            for chunk in batched(self.materials):
                results = run_tapir("GetPropertyValuesOfAttributes", {
                    "attributeIds": [{"attributeId": m["attributeId"]} for m in chunk],
                    "properties": [{"propertyId": prop["propertyId"]}],
                    "includeStatus": True}).get("propertyValuesForAttributes", [])
                for material, result in zip(chunk, results):
                    item = (result.get("propertyValues") or [{}])[0] if "error" not in result else result
                    if "error" in item:
                        unreadable += 1
                    material["available"] = bool(item.get("isAvailable", False))
                    material["editable"] = bool(item.get("isEditable", False))
                    material["value"] = item.get("propertyValue", {}).get("value", "") if material["available"] else None

        self._fill_tree()
        available = [m for m in self.materials if m["available"]]
        empty = [m for m in available if not m["value"]]
        self.count_label.configure(text=str(len(empty)), foreground="#c62828" if empty else "#1b7f3b")
        self.summary_label.configure(text=f"Building Material{'s' if len(empty) != 1 else ''} with an empty "
                                          f"{prop.get('propertyName', '')}")
        self.detail_label.configure(text=f"{len(available)} of {len(self.materials)} Building Materials have this property")
        if unreadable:
            self.window.set_status(f"Done, {unreadable} value(s) could not be read.", "warning")
        else:
            self.window.set_status("Every Building Material that has this property has a value." if not empty else "Done.",
                                   "ok" if not empty else "info")

    def _fill_tree(self) -> None:
        self.tree.delete(*self.tree.get_children())
        self.listed = [m for m in self.materials
                       if not self.only_empty.get() or (m["available"] and not m["value"])]
        for index, m in enumerate(self.listed):
            if not m["available"]:
                value, tag = NOT_AVAILABLE, "unavailable"
            elif not m["value"]:
                value, tag = EMPTY, "empty"
            else:
                value, tag = m["value"], ""
            self.tree.insert("", "end", iid=str(index), values=(m["name"], m["folder"], value), tags=(tag,) if tag else ())

    # ---- actions ----

    def set_value(self) -> None:
        prop = self._chosen_property()
        if prop is None or not self.materials:
            self.window.set_status("Press Refresh and pick a property first.", "warning")
            return
        value = self.new_value.get()
        if not value.strip():
            self.window.set_status("Type the new value first.", "warning")
            return
        rows = self.tree.selection()
        chosen = [self.listed[int(r)] for r in rows] if rows else [m for m in self.listed if m["available"] and not m["value"]]
        targets = [m for m in chosen if m["available"] and m["editable"]]
        skipped = len(chosen) - len(targets)
        if not targets:
            self.window.set_status("None of the chosen materials has an editable value of this property." if chosen
                                   else "No listed material has an empty value. Select the materials to change.", "warning")
            return

        lines = [f"• {m['name']}: {m['value'] or EMPTY} → {value}" for m in targets[:PREVIEW_LINES]]
        if len(targets) > PREVIEW_LINES:
            lines.append(f"… and {len(targets) - PREVIEW_LINES} more")
        if skipped:
            lines.append(f"\n{skipped} chosen material(s) are skipped: the property is not available or not editable for them.")
        question = f"Set {self._property_label(prop)} to \"{value}\" for {len(targets)} Building Material(s)?\n\n"
        if not self.window.ask_ok_cancel(SCRIPT_NAME, question + "\n".join(lines)):
            return

        failures = []
        with self.window.busy(f"Setting the value of {len(targets)} Building Materials..."):
            for chunk in batched(targets):
                try:
                    results = run_tapir("SetPropertyValuesOfAttributes", {"attributePropertyValues": [{
                        "attributeId": m["attributeId"],
                        "propertyId": prop["propertyId"],
                        "propertyValue": {"value": value},
                    } for m in chunk]}).get("executionResults", [])
                except ArchicadError as e:
                    results = [{"error": {"message": str(e)}} for _ in chunk]
                for material, error in zip(chunk, [item_errors([r]) for r in results]):
                    if error:
                        failures.append(f"{material['name']}: {error[0]}")
        for failure in failures:
            print(f"Not changed: {failure}")

        self.read_values()
        if failures:
            self.window.set_status(f"Changed {len(targets) - len(failures)} of {len(targets)} Building Materials. "
                                   f"First failure: {failures[0]}", "warning")
        else:
            self.window.set_status(f"Changed {len(targets)} Building Material(s)."
                                   + (f" {skipped} skipped." if skipped else ""), "ok")

    def export(self) -> None:
        prop = self._chosen_property()
        if prop is None or not self.materials:
            self.window.set_status("Press Refresh and pick a property first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the property values",
                                            initialfile=f"Building Materials {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        rows = [[m["name"], m["folder"], m["value"] if m["available"] else NOT_AVAILABLE] for m in self.listed]
        write_table(path, ["Building Material", "Folder", self._property_label(prop)], rows, "Building Materials")
        self.window.set_status(f"Exported {len(rows)} Building Material(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.refresh)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    BuildingMaterialPropertyAudit().run()
