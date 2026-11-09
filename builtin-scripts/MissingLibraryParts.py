# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "libraries-objects"
# changes = "model"
# ///

"""
Missing Library Parts

Lists the library parts the placed Objects, Lamps, Windows and Doors use but
the loaded libraries do not have (the elements Archicad shows as dots, e.g.
after migrating a project), with the elements that use each one, per story.
The elements can be selected in Archicad, or switched to a loaded library part
with ChangeLibraryPartOfElements. Nothing is changed before the list of
elements to change is confirmed; a report then shows the result of each one.
"""

import tkinter as tk
from tkinter import ttk

from utilities.archicad import (BATCH_SIZE, ArchicadError, batched, get_details_of_elements, get_elements, get_story_names,
                                run_tapir, select_elements)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Missing Library Parts"
# the element types that are placed from a library part and that GetMissingLibPartNames reads
ELEMENT_TYPES = ["Object", "Lamp", "Window", "Door"]
UNKNOWN_NAME = "(name not readable)"
CONFIRM_LIST_LENGTH = 25


def _name_variants(name: str) -> set[str]:
    """ A library part name as it is, in lowercase, and without its file extension. """
    name = name.strip().lower()
    variants = {name}
    if "." in name:
        variants.add(name.rsplit(".", 1)[0])
    return variants


class MissingLibraryParts:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the library parts the placed Objects, Lamps, Windows and Doors use but the loaded libraries "
            "do not have, with the elements that use them, per story. Select rows to select those elements in "
            "Archicad, or choose a loaded library part to replace the missing one with.",
            min_width=760, min_height=600)
        self.story_names: dict[int, str] = {}
        self.missing: list[dict] = []  # one entry per placed element whose library part is missing
        self.row_elements: dict[str, list[dict]] = {}  # tree row -> the elements under it
        self.loaded_parts: list[dict] = []
        self.candidates: dict[str, dict] = {}  # shown name -> loaded library part
        self._build_body()
        self.window.add_button("Select in Archicad", self.select_chosen)
        self.replace_button = self.window.add_button("Replace...", self.replace)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Elements", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        self.scope = tk.StringVar(value="project")
        for column, (value, text) in enumerate([("selection", "Selection"), ("story", "Current story"),
                                                ("project", "Whole project")]):
            ttk.Radiobutton(options, text=text, value=value, variable=self.scope,
                            command=self.window.guarded(self.refresh)).grid(
                row=0, column=column, sticky="w", padx=(0 if column == 0 else 10, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Press Refresh to look for missing library parts.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("type", "Type", 80, "w"),
            ("story", "Story", 140, "w"),
            ("count", "Elements", 80, "e"),
        ], height=12, show_tree=True)
        self.tree.heading("#0", text="Missing library part / story / element", anchor="w")
        self.tree.column("#0", width=330, stretch=True)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._update_candidates)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_chosen)())

        replace = ttk.LabelFrame(body, text="Replace with a loaded library part", padding=(10, 6))
        replace.grid(row=3, column=0, sticky="ew", pady=(10, 0))
        replace.columnconfigure(1, weight=1)
        ttk.Label(replace, text="Search:").grid(row=0, column=0, sticky="w")
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_a: self.window.guarded(self._update_candidates)())
        ttk.Entry(replace, textvariable=self.search).grid(row=0, column=1, sticky="ew", padx=(6, 0))
        ttk.Label(replace, text="Library part:").grid(row=1, column=0, sticky="w", pady=(6, 0))
        self.replacement = tk.StringVar()
        self.replacement_box = ttk.Combobox(replace, textvariable=self.replacement, state="readonly", height=20)
        self.replacement_box.grid(row=1, column=1, sticky="ew", padx=(6, 0), pady=(6, 0))
        self.replace_hint = ttk.Label(replace, text="Select the missing library parts to replace in the list.",
                                      style="Muted.TLabel")
        self.replace_hint.grid(row=2, column=0, columnspan=2, sticky="w", pady=(4, 0))
        switches = ttk.Frame(replace)
        switches.grid(row=3, column=0, columnspan=2, sticky="w", pady=(6, 0))
        self.keep_parameters = tk.BooleanVar(value=True)
        ttk.Checkbutton(switches, text="Keep the parameters with the same name",
                        variable=self.keep_parameters).pack(side="left")
        self.keep_size = tk.BooleanVar(value=True)
        ttk.Checkbutton(switches, text="Keep the size (A and B)", variable=self.keep_size).pack(side="left", padx=(16, 0))

    # ---- reading ----

    def refresh(self) -> None:
        scope = self.scope.get()
        warnings: list[str] = []
        with self.window.busy("Reading the loaded library parts..."):
            self.story_names = get_story_names()
            self.loaded_parts = run_tapir("GetAvailableLibraryParts").get("libraryParts", [])
            loaded_names: set[str] = set()
            for part in self.loaded_parts:
                loaded_names |= _name_variants(part.get("documentName", ""))
                loaded_names |= _name_variants(part.get("fileName", ""))
            loaded_names.discard("")

            self.window.set_status("Reading the elements...")
            elements = get_elements(scope, ELEMENT_TYPES)
            details = get_details_of_elements(elements, ["id", "type", "floorIndex", "details"])

            self.missing = []
            # GetMissingLibPartNames is only in newer Add-Ons; without it the name in the details is used.
            names_supported = True
            for chunk_start, chunk in zip(range(0, len(elements), BATCH_SIZE), batched(elements)):
                self.window.set_status(f"Reading the library part names... {chunk_start}/{len(elements)}")
                names: list[dict] = []
                if names_supported:
                    try:
                        names = run_tapir("GetMissingLibPartNames", {"elements": chunk}).get("libPartNames", [])
                    except ArchicadError:
                        names_supported = False
                names += [{"libPartName": ""}] * (len(chunk) - len(names))
                for index, (element, name_item) in enumerate(zip(chunk, names)):
                    detail = details[chunk_start + index] if chunk_start + index < len(details) else {}
                    if "error" in detail:
                        warnings.append(f"{element['elementId']['guid']}: {detail['error'].get('message', 'not readable')}")
                        continue
                    name = name_item.get("libPartName", "") if "error" not in name_item else ""
                    loaded_name = ((detail.get("details") or {}).get("libPart") or {}).get("name", "")
                    # A part is missing when no loaded library part has its name: neither the name in the
                    # element details nor the one from GetMissingLibPartNames (when the Add-On has it).
                    is_missing = not loaded_name or not (_name_variants(loaded_name) & loaded_names
                                                         or (name and _name_variants(name) & loaded_names))
                    if not is_missing:
                        continue
                    if "error" in name_item:
                        warnings.append(f"{detail.get('id') or element['elementId']['guid']}: "
                                        f"{name_item['error'].get('message', 'name not readable')}")
                    floor = detail.get("floorIndex")
                    self.missing.append({
                        "elementId": element["elementId"],
                        "part": name or loaded_name or UNKNOWN_NAME,
                        "type": detail.get("type", ""),
                        "id": detail.get("id") or "(no ID)",
                        "floor": floor,
                        "story": self.story_names.get(floor, f"Story {floor}") if floor is not None else "",
                    })

        self._fill_tree()
        parts = {(m["part"], m["type"]) for m in self.missing}
        where = {"story": "on the current story", "project": "in the project", "selection": "in the selection"}[scope]
        self.count_label.configure(text=str(len(parts)), foreground="#c62828" if parts else "#1b7f3b")
        self.summary_label.configure(text=f"missing library part{'s' if len(parts) != 1 else ''} {where}")
        self.detail_label.configure(text=f"used by {len(self.missing)} of the {len(elements)} Objects, Lamps, "
                                         f"Windows and Doors checked")
        self._update_candidates()
        if warnings:
            self.window.set_status(f"Done, with {len(warnings)} problem(s): {warnings[0]}", "warning")
            for warning in warnings:
                print(f"Warning: {warning}")
        elif parts:
            self.window.set_status("Select rows to select their elements or to replace their library part.")
        else:
            self.window.set_status("Every library part is loaded.", "ok")

    def _fill_tree(self) -> None:
        self.tree.delete(*self.tree.get_children())
        self.row_elements = {}
        groups: dict[tuple[str, str], dict[int | None, list[dict]]] = {}
        for entry in self.missing:
            groups.setdefault((entry["part"], entry["type"]), {}).setdefault(entry["floor"], []).append(entry)
        for part_number, ((part, element_type), stories) in enumerate(sorted(groups.items())):
            part_row = f"p{part_number}"
            part_entries = [e for entries in stories.values() for e in entries]
            self.tree.insert("", "end", iid=part_row, text=part, open=False,
                             values=(element_type, f"{len(stories)} stor{'ies' if len(stories) != 1 else 'y'}",
                                     len(part_entries)))
            self.row_elements[part_row] = part_entries
            for story_number, floor in enumerate(sorted(stories, key=lambda f: (f is None, f or 0))):
                entries = sorted(stories[floor], key=lambda e: e["id"])
                story_row = f"{part_row}s{story_number}"
                self.tree.insert(part_row, "end", iid=story_row, text=entries[0]["story"] or "(no story)",
                                 values=(element_type, entries[0]["story"], len(entries)))
                self.row_elements[story_row] = entries
                for element_number, entry in enumerate(entries):
                    element_row = f"{story_row}e{element_number}"
                    self.tree.insert(story_row, "end", iid=element_row, text=entry["id"],
                                     values=(element_type, entry["story"], ""))
                    self.row_elements[element_row] = [entry]

    # ---- choosing ----

    def _chosen(self) -> list[dict]:
        """ The elements under the selected rows, each once; all listed ones when no row is selected. """
        rows = self.tree.selection()
        if not rows:
            return list(self.missing)
        chosen: dict[str, dict] = {}
        for row in rows:
            for entry in self.row_elements.get(row, []):
                chosen.setdefault(entry["elementId"]["guid"], entry)
        return list(chosen.values())

    def _update_candidates(self) -> None:
        """ Lists the loaded library parts of the type of the chosen elements in the replacement box. """
        types = {e["type"] for e in self._chosen()} if self.tree.selection() else set()
        self.candidates = {}
        if len(types) == 1:
            element_type = next(iter(types))
            search = self.search.get().strip().lower()
            for part in sorted(self.loaded_parts, key=lambda p: p.get("documentName", "").lower()):
                if part.get("typeId") != element_type or part.get("type", "Object") != "Object":
                    continue
                name = part.get("documentName") or part.get("fileName", "")
                if search and search not in name.lower() and search not in part.get("fileName", "").lower():
                    continue
                shown = name if name not in self.candidates else f"{name}  ({part.get('fileName', '')})"
                self.candidates[shown] = part
            self.replace_hint.configure(text=f"{len(self.candidates)} loaded {element_type} library part(s)"
                                             f"{' match the search' if search else ''}.")
        elif types:
            self.replace_hint.configure(text="The selected rows hold elements of several types (" +
                                             ", ".join(sorted(types)) + "); select one type to replace.")
        else:
            self.replace_hint.configure(text="Select the missing library parts to replace in the list.")
        self.replacement_box.configure(values=list(self.candidates))
        if self.replacement.get() not in self.candidates:
            self.replacement.set("")
        self.replace_button.state(["!disabled"] if self.candidates else ["disabled"])

    # ---- actions ----

    def select_chosen(self) -> None:
        elements = [{"elementId": e["elementId"]} for e in self._chosen()]
        if not elements:
            self.window.set_status("There is nothing to select.", "warning")
            return
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def replace(self) -> None:
        if not self.tree.selection():
            self.window.set_status("Select the missing library parts to replace in the list first.", "warning")
            return
        part = self.candidates.get(self.replacement.get())
        if part is None:
            self.window.set_status("Choose the library part to replace them with first.", "warning")
            return
        elements = self._chosen()
        if len({e["type"] for e in elements}) != 1:
            self.window.set_status("Select elements of one type to replace.", "warning")
            return
        new_name = part.get("documentName") or part.get("fileName", "")
        lines = [f"{e['id']}  ·  {e['story']}  ·  {e['part']}" for e in elements[:CONFIRM_LIST_LENGTH]]
        if len(elements) > CONFIRM_LIST_LENGTH:
            lines.append(f"... and {len(elements) - CONFIRM_LIST_LENGTH} more")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Change the library part of {len(elements)} element(s) "
                                                      f"to '{new_name}'?\n\n" + "\n".join(lines) +
                                                      "\n\nYou can undo it in Archicad."):
            return

        results: list[dict] = []
        with self.window.busy(f"Replacing the library part of {len(elements)} element(s)..."):
            for chunk in batched(elements):
                results += run_tapir("ChangeLibraryPartOfElements", {"elementsWithLibraryPart": [{
                    "elementId": e["elementId"],
                    "libraryPart": {"index": part["index"]},
                    "keepParameters": self.keep_parameters.get(),
                    "keepSize": self.keep_size.get(),
                } for e in chunk]}).get("executionResults", [])

        report = []
        for entry, result in zip(elements, results):
            error = result.get("error") if isinstance(result, dict) else None
            ok = not error and result.get("success", True) is not False
            message = "Replaced" if ok else (error or {}).get("message", "Failed")
            report.append((entry, ok, message))
            print(f"{entry['id']} ({entry['story']}): {entry['part']} -> {new_name}: {message}")
        for entry in elements[len(results):]:
            report.append((entry, False, "No result returned"))
        self._show_report(new_name, report)

        failed = sum(not ok for _e, ok, _m in report)
        try:
            self.refresh()
        except ArchicadError as e:
            self.window.set_status(f"Replaced, but the list could not be refreshed: {e}", "warning")
            return
        if failed:
            self.window.set_status(f"Replaced {len(report) - failed} of {len(report)} element(s); "
                                   f"{failed} failed, see the report.", "warning")
        else:
            self.window.set_status(f"Replaced the library part of {len(report)} element(s).", "ok")

    def _show_report(self, new_name: str, report: list[tuple[dict, bool, str]]) -> None:
        dialog = tk.Toplevel(self.window.root)
        dialog.title(f"{SCRIPT_NAME} - report")
        dialog.minsize(640, 360)
        dialog.attributes("-topmost", self.window.keep_on_top.get())
        dialog.columnconfigure(0, weight=1)
        dialog.rowconfigure(1, weight=1)
        replaced = sum(ok for _e, ok, _m in report)
        ttk.Label(dialog, padding=(12, 10, 12, 6),
                  text=f"Changed to '{new_name}': {replaced} of {len(report)} element(s).").grid(row=0, column=0, sticky="w")
        tree = make_tree(dialog, [
            ("element", "Element ID", 140, "w"),
            ("story", "Story", 120, "w"),
            ("old", "Missing library part", 180, "w"),
            ("result", "Result", 200, "w"),
        ], height=12)
        tree.container.grid(row=1, column=0, sticky="nsew", padx=12)
        tree.tag_configure("failed", foreground="#c62828")
        for index, (entry, ok, message) in enumerate(report):
            tree.insert("", "end", iid=str(index), tags=() if ok else ("failed",),
                        values=(entry["id"], entry["story"], entry["part"], message))
        ttk.Button(dialog, text="Close", command=dialog.destroy).grid(row=2, column=0, sticky="e", padx=12, pady=10)

    def run(self) -> None:
        self.window.run(on_start=self.refresh)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    MissingLibraryParts().run()
