# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "documentation-layouts"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "model"
# ///

"""
Drawing Status Report

Lists every Drawing placed on the layouts of the Layout Book with the columns
of Archicad's Drawing Manager: status, update type, stored in project, source
and path. Export the list to Excel or CSV, or update the drawings whose source
has changed: the script shows which drawings it will update and asks before
updating them, then reports the drawings it could not update. Reading opens
each layout in turn (Archicad gives a drawing's data only from its own layout)
and then goes back to the window that was active. Based on the
get_drawing_manager_data.py workflow example.
"""

import datetime
import os
import tkinter as tk
from tkinter import filedialog, ttk

from utilities.archicad import ArchicadError, get_details_of_elements, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Drawing Status Report"
STATUS_TEXT = {"UpToDate": "OK", "Modified": "Modified", "Unknown": "Unknown"}
SOURCE_TEXT = {
    "InternalView": "Internal view", "ExternalView": "External view", "Drawing": "Drawing file",
    "DXF_DWG": "DXF/DWG", "PDF": "PDF", "Image": "Image", "PlotMaker": "PlotMaker", "AddOn": "Add-On",
}
PREVIEW_LINES = 15


class DrawingStatusReport:
    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the drawings of the layouts with their Drawing Manager status. Select drawings and press "
            "Update drawings to update them; with nothing selected, the modified drawings in the list are updated.",
            min_width=760, min_height=480)
        self.drawings: list[dict] = []
        self.listed: list[dict] = []
        self._build_body()
        self.window.add_button("Export...", self.export)
        self.window.add_button("Update drawings...", self.update_drawings)
        self.window.add_button("Refresh", self.refresh, primary=True)

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(2, weight=1)

        options = ttk.Frame(body)
        options.grid(row=0, column=0, sticky="ew")
        self.only_outdated = tk.BooleanVar(value=False)
        ttk.Checkbutton(options, text="Only drawings that are not up to date", variable=self.only_outdated,
                        command=self._fill_tree).grid(row=0, column=0, sticky="w")

        summary = ttk.Frame(body, padding=(0, 10, 0, 6))
        summary.grid(row=1, column=0, sticky="ew")
        self.count_label = ttk.Label(summary, text="–", style="Big.TLabel")
        self.count_label.grid(row=0, column=0, rowspan=2, sticky="w", padx=(0, 10))
        self.summary_label = ttk.Label(summary, text="Press Refresh to read the drawings.")
        self.summary_label.grid(row=0, column=1, sticky="sw")
        self.detail_label = ttk.Label(summary, text="", style="Muted.TLabel")
        self.detail_label.grid(row=1, column=1, sticky="nw")

        self.tree = make_tree(body, [
            ("layout", "Layout", 190, "w"),
            ("title", "Custom name", 150, "w"),
            ("status", "Status", 80, "w"),
            ("update", "Update type", 85, "w"),
            ("stored", "Stored in project", 110, "w"),
            ("source", "Source", 100, "w"),
            ("path", "Path", 200, "w"),
        ], height=12)
        self.tree.container.grid(row=2, column=0, sticky="nsew")
        self.tree.tag_configure("Modified", foreground="#c62828")
        self.tree.tag_configure("Unknown", foreground="#a15c00")

    # ---- reading ----

    def refresh(self) -> None:
        drawings = []
        details = []
        failed_layouts = 0
        with self.window.busy("Reading the layouts..."):
            layouts = run_tapir("GetDatabases", {"types": ["Layout"]}).get("databases", [])
            # GetDetailsOfElements returns the details of the current database's elements only
            # (a drawing of another layout comes back with fromOtherDatabase and no details),
            # so each layout is opened before its drawings are read, and the user's window is
            # brought back afterwards.
            start_database = run_tapir("GetCurrentWindowType").get("databaseId")
            try:
                for index, layout in enumerate(layouts):
                    self.window.set_status(f"Reading the drawings of the layouts... {index + 1} / {len(layouts)}")
                    name = " ".join(part for part in (layout.get("referenceId", ""), layout.get("name", "")) if part)
                    try:
                        run_tapir("ChangeWindow", {"databaseId": layout["databaseId"]})
                        elements = run_tapir("GetElementsByType", {
                            "elementType": "Drawing", "databases": [{"databaseId": layout["databaseId"]}]}).get("elements", [])
                        layout_details = get_details_of_elements(
                            [{"elementId": e["elementId"]} for e in elements], ["details"])
                    except ArchicadError as e:
                        failed_layouts += 1
                        print(f"Cannot read the drawings of layout {name}: {e}")
                        continue
                    drawings += [{"elementId": e["elementId"], "layout": name} for e in elements]
                    details += layout_details
            finally:
                if start_database is not None:
                    try:
                        run_tapir("ChangeWindow", {"databaseId": start_database})
                    except ArchicadError as e:
                        print(f"Cannot go back to the window the script started in: {e}")

        self.drawings = []
        unreadable = 0
        for drawing, detail in zip(drawings, details):
            if "error" in detail or detail.get("fromOtherDatabase"):
                unreadable += 1
                print(f"Cannot read the Drawing Manager data of a drawing on layout {drawing['layout']}.")
                continue
            d = detail.get("details", {})
            self.drawings.append({
                **drawing,
                "title": d.get("customName", ""),
                "status": d.get("status", "Unknown"),
                "manualUpdate": d.get("manualUpdate", False),
                "storedInProject": d.get("storedInProject", False),
                "source": SOURCE_TEXT.get(d.get("linkType", ""), d.get("linkType", "")),
                "path": d.get("linkPath", ""),
            })
        self.drawings.sort(key=lambda d: (d["layout"].lower(), d["title"].lower()))
        self._fill_tree()

        outdated = sum(1 for d in self.drawings if d["status"] != "UpToDate")
        self.count_label.configure(text=str(outdated), foreground="#c62828" if outdated else "#1b7f3b")
        self.summary_label.configure(text=f"drawing{'s' if outdated != 1 else ''} not up to date")
        self.detail_label.configure(text=f"{len(self.drawings)} drawings on {len(layouts)} layouts")
        problems = []
        if failed_layouts:
            problems.append(f"{failed_layouts} layout(s) could not be read")
        if unreadable:
            problems.append(f"{unreadable} drawing(s) could not be read")
        if problems:
            self.window.set_status("Done, but " + " and ".join(problems) + ". See the palette output.", "warning")
        else:
            self.window.set_status("All drawings are up to date." if not outdated else "Done.",
                                   "ok" if not outdated else "info")

    def _fill_tree(self) -> None:
        self.tree.delete(*self.tree.get_children())
        self.listed = [d for d in self.drawings if not self.only_outdated.get() or d["status"] != "UpToDate"]
        for index, d in enumerate(self.listed):
            self.tree.insert("", "end", iid=str(index), tags=(d["status"],), values=(
                d["layout"], d["title"] or "–", STATUS_TEXT.get(d["status"], d["status"]),
                "Manual" if d["manualUpdate"] else "Auto", "Yes" if d["storedInProject"] else "No",
                d["source"], d["path"]))

    # ---- actions ----

    def _label(self, drawing: dict) -> str:
        return f"{drawing['layout']}: {drawing['title']}" if drawing["title"] else drawing["layout"]

    def update_drawings(self) -> None:
        rows = self.tree.selection()
        if rows:
            targets = [self.listed[int(r)] for r in rows]
        else:
            targets = [d for d in self.listed if d["status"] == "Modified"]
        if not targets:
            self.window.set_status("No modified drawings in the list. Select the drawings to update.", "warning")
            return

        lines = [f"• {self._label(d)} ({STATUS_TEXT.get(d['status'], d['status'])})" for d in targets[:PREVIEW_LINES]]
        if len(targets) > PREVIEW_LINES:
            lines.append(f"… and {len(targets) - PREVIEW_LINES} more")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, f"Update {len(targets)} drawing(s)?\n\n" + "\n".join(lines)):
            return

        failures = []
        with self.window.busy("Updating the drawings..."):
            for index, drawing in enumerate(targets):
                self.window.set_status(f"Updating the drawings... {index + 1} / {len(targets)}")
                # one drawing per call: UpdateDrawings reports one result for all the drawings it is given
                try:
                    run_tapir("UpdateDrawings", {"elements": [{"elementId": drawing["elementId"]}]})
                except ArchicadError as e:
                    failures.append(f"{self._label(drawing)}: {e}")
        for failure in failures:
            print(f"Not updated: {failure}")

        self.refresh()
        if failures:
            self.window.set_status(f"Updated {len(targets) - len(failures)} of {len(targets)} drawings. "
                                   f"First failure: {failures[0]}", "warning")
        else:
            self.window.set_status(f"Updated {len(targets)} drawing(s).", "ok")

    def export(self) -> None:
        if not self.drawings:
            self.window.set_status("Press Refresh first.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d %H-%M")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Export the drawing list",
                                            initialfile=f"Drawings {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        rows = [[d["layout"], d["title"], STATUS_TEXT.get(d["status"], d["status"]),
                 "Manual" if d["manualUpdate"] else "Auto", "Yes" if d["storedInProject"] else "No",
                 d["source"], d["path"], d["elementId"]["guid"]] for d in self.listed]
        write_table(path, ["Layout", "Custom name", "Status", "Update type", "Stored in project", "Source", "Path", "GUID"],
                    rows, "Drawings")
        self.window.set_status(f"Exported {len(rows)} drawing(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.refresh)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    DrawingStatusReport().run()
