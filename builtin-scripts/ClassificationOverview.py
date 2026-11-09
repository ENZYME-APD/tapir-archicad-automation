# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "classification-attributes"
# changes = "files"
# ///

"""
Classification Overview

Shows how many elements are classified with each item of a classification
system, as a tree that follows the system's hierarchy, plus the elements that
have no classification in it. Select items to highlight their elements,
double-click one to select its elements in Archicad, and export the counts to
Excel or CSV. Based on the get_elements_by_classification.py workflow example.
"""

import datetime
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import get_elements, run_command, run_tapir, select_elements
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Classification Overview"
UNCLASSIFIED = "__unclassified__"
HIGHLIGHT_COLOR = [0, 130, 200, 255]
DIMMED_COLOR = [200, 200, 200, 160]


class ClassificationOverview:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Counts the elements classified with each item of a classification system. "
            "Select items to highlight their elements; double-click one to select its elements in Archicad.",
            min_width=560, min_height=520)
        self.systems: list[dict] = []
        # per tree row: {"id", "name", "parent", "elements"} - elements are those classified with that item only
        self.items: dict[str, dict] = {}
        self.order: list[str] = []
        self._build_body()
        self.window.add_button("Export...", self.export)
        self.window.add_button("Select in Archicad", self.select_listed)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.LabelFrame(body, text="Classification system", padding=(10, 6))
        options.grid(row=0, column=0, sticky="ew")
        options.columnconfigure(0, weight=1)
        self.system = tk.StringVar()
        self.system_box = ttk.Combobox(options, textvariable=self.system, state="readonly")
        self.system_box.grid(row=0, column=0, sticky="ew")
        self.system_box.bind("<<ComboboxSelected>>", lambda _e: self.window.guarded(self.refresh)())
        self.hide_empty = tk.BooleanVar(value=True)
        ttk.Checkbutton(options, text="Hide items without elements", variable=self.hide_empty,
                        command=self._fill_tree).grid(row=1, column=0, sticky="w", pady=(6, 0))

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Pick a classification system.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [("own", "Elements", 80, "e"), ("total", "With sub-items", 100, "e")],
                              height=12, show_tree=True)
        self.tree.heading("#0", text="Classification", anchor="w")
        self.tree.column("#0", width=340, stretch=True)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("empty", foreground="#777777")
        self.tree.tag_configure("unclassified", foreground="#a15c00")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self.window.guarded(self._highlight)())
        self.tree.bind("<Double-1>", lambda _e: self.window.guarded(self.select_clicked)())

    # ---- reading ----

    def load_systems(self) -> None:
        with self.window.busy("Reading the classification systems..."):
            self.systems = run_command("GetAllClassificationSystems").get("classificationSystems", [])
        names = [f"{s.get('name', '')} {s.get('version', '')}".strip() for s in self.systems]
        self.system_box.configure(values=names)
        if not names:
            self.window.set_status("The project has no classification systems.", "warning")
            return
        self.system_box.current(0)
        self.refresh()

    def _chosen_system(self) -> dict | None:
        index = self.system_box.current()
        return self.systems[index] if 0 <= index < len(self.systems) else None

    def _collect(self, classification_items: list[dict], parent: str) -> None:
        for wrapper in classification_items:
            item = wrapper["classificationItem"]
            guid = item["classificationItemId"]["guid"]
            self.items[guid] = {"id": item.get("id", ""), "name": item.get("name", ""), "parent": parent,
                                "classificationItemId": item["classificationItemId"], "elements": []}
            self.order.append(guid)
            self._collect(item.get("children", []), guid)

    def refresh(self) -> None:
        system = self._chosen_system()
        if system is None:
            self.window.set_status("Pick a classification system first.", "warning")
            return
        self.items, self.order = {}, []
        failed = []
        with self.window.busy("Reading the classification items..."):
            tree = run_command("GetAllClassificationsInSystem", {"classificationSystemId": system["classificationSystemId"]})
            self._collect(tree.get("classificationItems", []), "")
            if self.order:
                self.window.set_status("Reading the classified elements...")
                results = run_tapir("GetElementsByClassification", {
                    "classificationItemIds": [{"classificationItemId": self.items[g]["classificationItemId"]}
                                              for g in self.order]}).get("elementsOfClassificationItems", [])
                for guid, result in zip(self.order, results):
                    if "error" in result:
                        failed.append(self.items[guid]["id"] or self.items[guid]["name"])
                    else:
                        self.items[guid]["elements"] = result.get("elements", [])

            self.window.set_status("Reading the unclassified elements...")
            classified = {e["elementId"]["guid"] for item in self.items.values() for e in item["elements"]}
            unclassified = [e for e in get_elements("project") if e["elementId"]["guid"] not in classified]
        self.items[UNCLASSIFIED] = {"id": "", "name": "(not classified in this system)", "parent": "",
                                    "elements": unclassified}

        self._fill_tree()
        count = len(classified)
        self.count_label.configure(text=str(count))
        self.summary_label.configure(text=f"element{'s' if count != 1 else ''} classified in {system.get('name', '')}")
        used = sum(1 for g in self.order if self.items[g]["elements"])
        self.detail_label.configure(text=f"{used} of {len(self.order)} items used, {len(unclassified)} elements not classified")
        if failed:
            self.window.set_status(f"Done, {len(failed)} item(s) could not be read, the first: {failed[0]}", "warning")
        else:
            self.window.set_status("Done.", "ok")

    def _totals(self) -> dict[str, int]:
        totals = {g: len(self.items[g]["elements"]) for g in self.order}
        # children come after their parent in self.order, so going backwards adds each to its parent once complete
        for guid in reversed(self.order):
            parent = self.items[guid]["parent"]
            if parent:
                totals[parent] += totals[guid]
        return totals

    def _fill_tree(self) -> None:
        self.tree.delete(*self.tree.get_children())
        if not self.items:
            return
        totals = self._totals()
        for guid in self.order:
            item = self.items[guid]
            if self.hide_empty.get() and not totals[guid]:
                continue
            label = f"{item['id']}  {item['name']}".strip() if item["id"] else item["name"]
            self.tree.insert(item["parent"], "end", iid=guid, text=label, open=True,
                             values=(len(item["elements"]), totals[guid]),
                             tags=() if totals[guid] else ("empty",))
        unclassified = self.items[UNCLASSIFIED]
        self.tree.insert("", "end", iid=UNCLASSIFIED, text=unclassified["name"], tags=("unclassified",),
                         values=(len(unclassified["elements"]), len(unclassified["elements"])))

    # ---- elements of the rows ----

    def _elements_under(self, guid: str) -> list[dict]:
        """ The elements of the row and of every item below it. """
        elements = list(self.items[guid]["elements"])
        for child in self.tree.get_children(guid):
            elements += self._elements_under(child)
        return elements

    def _elements_of_rows(self, rows) -> list[dict]:
        seen, elements = set(), []
        for row in rows:
            for element in self._elements_under(row):
                if element["elementId"]["guid"] not in seen:
                    seen.add(element["elementId"]["guid"])
                    elements.append({"elementId": element["elementId"]})
        return elements

    def _highlight(self) -> None:
        elements = self._elements_of_rows(self.tree.selection())
        run_tapir("HighlightElements", {
            "elements": elements,
            "highlightedColors": [HIGHLIGHT_COLOR for _ in elements],
            "wireframe3D": False,
            "nonHighlightedColor": DIMMED_COLOR,
        })

    def select_listed(self) -> None:
        rows = self.tree.selection()
        if not rows:
            self.window.set_status("Select items in the list first.", "warning")
            return
        elements = self._elements_of_rows(rows)
        select_elements(elements)
        self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def select_clicked(self) -> None:
        row = self.tree.focus()
        if row:
            elements = self._elements_of_rows([row])
            select_elements(elements)
            self.window.set_status(f"Selected {len(elements)} element(s) in Archicad.", "ok")

    def export(self) -> None:
        system = self._chosen_system()
        if system is None or not self.items:
            self.window.set_status("Press Refresh first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the classification counts",
                                            initialfile=f"Classification {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        totals = self._totals()
        rows = []
        for guid in self.order:
            item = self.items[guid]
            parent = self.items[item["parent"]]["id"] if item["parent"] else ""
            rows.append([item["id"], item["name"], parent, len(item["elements"]), totals[guid]])
        rows.append(["", self.items[UNCLASSIFIED]["name"], "", len(self.items[UNCLASSIFIED]["elements"]),
                     len(self.items[UNCLASSIFIED]["elements"])])
        write_table(path, ["ID", "Name", "Parent ID", "Elements", "With sub-items"], rows, system.get("name", "Classification"))
        self.window.set_status(f"Exported {len(rows)} row(s) to {os.path.basename(path)}.", "ok")

    def clear_highlight(self) -> None:
        run_tapir("HighlightElements", {"elements": [], "highlightedColors": []})

    def run(self) -> None:
        self.window.run(on_start=self.load_systems, on_close=self.clear_highlight)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    ClassificationOverview().run()
