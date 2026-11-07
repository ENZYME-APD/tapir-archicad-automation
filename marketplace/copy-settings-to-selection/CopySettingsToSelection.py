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

'''
Copy Settings to Selection

Copies the settings of one element to the selected elements of the same type,
without their geometry: the eyedropper and syringe for many elements at once.
The source's settings are saved as a temporary Favorite, applied to the
selected elements with ApplyFavoritesToElements, and the Favorite is deleted
again.

for tapir
'''

import uuid
import tkinter as tk
from tkinter import ttk

from utilities.archicad import ArchicadError, get_details_of_elements, item_errors, run_tapir
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Copy Settings to Selection"
PREVIEW_LINES = 15

# Fields of the targets that ApplyFavoritesToElements copies back after applying the Favorite,
# and the element types each group is sent for (None: every type). 'height' is a field of both
# openings and Walls/Columns, so each group is only sent for its own types.
KEEP_ID = ["id"]
KEEP_LAYER = ["layerIndex"]
KEEP_HEIGHT_AND_TOP = ["height", "topOffset", "relativeTopStory"]
KEEP_OPENING_SIZE = ["width", "height", "sillHeight"]  # a Skylight has no sillHeight: ignored
KEEP_REFERENCE_LINE = ["referenceLineLocation"]
HEIGHT_AND_TOP_TYPES = {"Wall", "Column"}
OPENING_TYPES = {"Window", "Door", "Skylight"}
REFERENCE_LINE_TYPES = {"Wall"}


class CopySettingsToSelection:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Pick a source element in Archicad, select the elements to change, and apply. "
            "The selected elements of the source's type get its settings; their position, "
            "story and the sizes kept below stay as they are.",
            min_width=520, min_height=460)
        self.source: dict | None = None  # {"elementId", "type", "id"}
        self.targets: list[dict] = []  # {"elementId", "type", "id"}
        self.skipped: list[dict] = []
        self._build_body()
        self.window.add_button("Pick source...", self.pick_source)
        self.window.add_button("Read selection", self.read_selection)
        self.apply_button = self.window.add_button("Apply...", self.apply, primary=True)
        self.apply_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        source = ttk.LabelFrame(body, text="Source", padding=(10, 6))
        source.grid(row=0, column=0, sticky="ew")
        source.columnconfigure(0, weight=1)
        self.source_label = ttk.Label(source, text="No source yet. Press Pick source... and click an element in Archicad.")
        self.source_label.grid(row=0, column=0, sticky="w")

        options = ttk.LabelFrame(body, text="Apply", padding=(10, 6))
        options.grid(row=1, column=0, sticky="ew", pady=(8, 0))
        self.apply_settings = tk.BooleanVar(value=True)
        self.apply_classifications = tk.BooleanVar(value=True)
        self.apply_categories = tk.BooleanVar(value=True)
        self.apply_properties = tk.BooleanVar(value=True)
        for column, (text, variable) in enumerate([
                ("Settings", self.apply_settings),
                ("Classifications", self.apply_classifications),
                ("Categories", self.apply_categories),
                ("Properties", self.apply_properties)]):
            ttk.Checkbutton(options, text=text, variable=variable).grid(row=0, column=column, sticky="w", padx=(0, 10))

        ttk.Label(options, text="Keep on the selected elements:").grid(row=1, column=0, columnspan=4, sticky="w", pady=(8, 0))
        self.keep_id = tk.BooleanVar(value=True)
        self.keep_layer = tk.BooleanVar(value=False)
        self.keep_height_and_top = tk.BooleanVar(value=True)
        self.keep_opening_size = tk.BooleanVar(value=True)
        self.keep_reference_line = tk.BooleanVar(value=True)
        for row, (text, variable) in enumerate([
                ("Element ID", self.keep_id),
                ("Layer", self.keep_layer),
                ("Height and top link (Walls, Columns)", self.keep_height_and_top),
                ("Size and sill height (Windows, Doors, Skylights)", self.keep_opening_size),
                ("Reference line location (Walls)", self.keep_reference_line)], start=2):
            ttk.Checkbutton(options, text=text, variable=variable).grid(row=row, column=0, columnspan=4, sticky="w", padx=(10, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=2, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="selected elements will change")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("id", "Element ID", 160, "w"),
            ("type", "Type", 120, "w"),
            ("status", "", 200, "w"),
        ], height=8)
        self.tree.container.grid(row=3, column=0, sticky="nsew")

    def _keep_fields(self) -> list[str]:
        element_type = self.source["type"] if self.source else None
        fields = []
        for variable, names, types in [(self.keep_id, KEEP_ID, None),
                                       (self.keep_layer, KEEP_LAYER, None),
                                       (self.keep_height_and_top, KEEP_HEIGHT_AND_TOP, HEIGHT_AND_TOP_TYPES),
                                       (self.keep_opening_size, KEEP_OPENING_SIZE, OPENING_TYPES),
                                       (self.keep_reference_line, KEEP_REFERENCE_LINE, REFERENCE_LINE_TYPES)]:
            if variable.get() and (types is None or element_type in types):
                fields += [name for name in names if name not in fields]
        return fields

    @staticmethod
    def _describe(element: dict) -> str:
        return f"{element['type']} {element['id'] or '(no ID)'}"

    def pick_source(self) -> None:
        with self.window.busy("Click the source element in Archicad (Escape cancels)..."):
            picked = run_tapir("GetElementFromUser", {"prompt": "Click the element to copy the settings from"})
        if picked.get("cancelled"):
            self.window.set_status("No source picked.", "warning")
            return
        details = get_details_of_elements([{"elementId": picked["elementId"]}], ["id", "type"])
        detail = details[0] if details else {}
        self.source = {
            "elementId": picked["elementId"],
            "type": detail.get("type") or picked.get("elementType", "Unknown"),
            "id": detail.get("id", ""),
        }
        self.source_label.configure(text=self._describe(self.source))
        self.read_selection()

    def read_selection(self) -> None:
        with self.window.busy("Reading the selection..."):
            selected = run_tapir("GetSelectedElements").get("elements", [])
            details = get_details_of_elements(selected, ["id", "type"]) if selected else []

        source_guid = self.source["elementId"]["guid"].lower() if self.source else None
        self.targets, self.skipped = [], []
        for element, detail in zip(selected, details):
            if element["elementId"]["guid"].lower() == source_guid:
                continue
            item = {"elementId": element["elementId"], "type": detail.get("type", "Unknown"), "id": detail.get("id", "")}
            if "error" in detail:
                item["reason"] = "could not be read"
                self.skipped.append(item)
            elif self.source and item["type"] != self.source["type"]:
                item["reason"] = "skipped: different type"
                self.skipped.append(item)
            else:
                self.targets.append(item)
        if not self.source:
            # without a source nothing can be applied yet: every element is only listed
            self.skipped, self.targets = self.targets + self.skipped, []
            for item in self.skipped:
                item.setdefault("reason", "")
        self._show()

    def _show(self) -> None:
        self.tree.delete(*self.tree.get_children())
        for index, item in enumerate(self.targets):
            self.tree.insert("", "end", iid=f"t{index}", values=(item["id"] or "(no ID)", item["type"], "will change"))
        for index, item in enumerate(self.skipped):
            self.tree.insert("", "end", iid=f"s{index}", values=(item["id"] or "(no ID)", item["type"], item["reason"]))

        count = len(self.targets)
        self.count_label.configure(text=str(count) if self.source else "–")
        self.summary_label.configure(text=f"selected element{'s' if count != 1 else ''} will change")
        if self.skipped and self.source:
            self.detail_label.configure(text=f"{len(self.skipped)} skipped: not of the source's type ({self.source['type']}) or not readable")
        else:
            self.detail_label.configure(text="")
        self.apply_button.state(["!disabled"] if self.source and count else ["disabled"])
        if not self.source:
            self.window.set_status("Pick the source element.", "info")
        elif not count:
            self.window.set_status(f"Select the {self.source['type']} elements to change in Archicad, then press Read selection.", "warning")
        else:
            self.window.set_status("Check the list, then press Apply...", "info")

    def _confirm(self) -> bool:
        lines = [f"Source: {self._describe(self.source)}", "",
                 f"{len(self.targets)} element(s) will get its settings:"]
        lines += [f"  {self._describe(t)}" for t in self.targets[:PREVIEW_LINES]]
        if len(self.targets) > PREVIEW_LINES:
            lines.append(f"  ... and {len(self.targets) - PREVIEW_LINES} more")
        if self.skipped:
            lines += ["", f"{len(self.skipped)} selected element(s) are skipped (different type or could not be read)."]
        keep = self._keep_fields()
        lines += ["", "Kept: " + (", ".join(keep) if keep else "nothing")]
        return self.window.ask_ok_cancel(SCRIPT_NAME, "\n".join(lines))

    def apply(self) -> None:
        if not self.source or not self.targets:
            return
        # the selection may have changed since it was read
        self.read_selection()
        if not self.targets or not self._confirm():
            return

        favorite = f"Tapir temporary {uuid.uuid4().hex[:8]}"
        with self.window.busy("Copying the settings..."):
            created = run_tapir("CreateFavoritesFromElements", {
                "favoritesFromElements": [{"elementId": self.source["elementId"], "favorite": favorite}],
            }).get("executionResults", [])
            created_errors = item_errors(created)
            if not created or created_errors:
                raise ArchicadError("The settings of the source could not be saved as a Favorite: "
                                    + (created_errors[0] if created_errors else "no result"))
            try:
                results = run_tapir("ApplyFavoritesToElements", {
                    "favoritesToApply": [{"elementId": t["elementId"], "favorite": favorite} for t in self.targets],
                    "applySettings": self.apply_settings.get(),
                    "applyClassifications": self.apply_classifications.get(),
                    "applyCategories": self.apply_categories.get(),
                    "applyProperties": self.apply_properties.get(),
                    "keepFields": self._keep_fields(),
                }).get("executionResults", [])
            finally:
                deleted = item_errors(run_tapir("DeleteFavorites", {"favorites": [favorite]}).get("executionResults", []))
                if deleted:
                    print(f"The temporary Favorite '{favorite}' could not be deleted: {deleted[0]}")

        failed = []
        for target, result in zip(self.targets, results):
            errors = item_errors([result])
            if errors:
                failed.append((target, errors[0]))
                print(f"Not changed: {self._describe(target)}: {errors[0]}")
        changed = len(self.targets) - len(failed)
        if failed:
            self.window.set_status(f"Changed {changed}, {len(failed)} not changed: {failed[0][1]}", "warning")
        elif deleted:
            self.window.set_status(f"Changed {changed} element(s). Delete the Favorite '{favorite}' by hand.", "warning")
        else:
            self.window.set_status(f"Changed {changed} element(s).", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.read_selection)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    CopySettingsToSelection().run()
