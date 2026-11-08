# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

# /// tapir
# category = "documentation-layouts"
# archicad-versions = [25, 26, 27, 28, 29, 30]
# min-tapir-version = "1.8.0"
# changes = "files"
# ///

"""
Sheet Index Export

Walks the Layout Book and lists one row per layout: its subset, number, name,
paper size, the current revision and that revision's changes. The list can be
saved as an Excel or CSV file, or copied to the clipboard to paste into a
spreadsheet. It only reads the project; nothing in Archicad is changed.
"""

import datetime
import os
from tkinter import filedialog

from utilities.archicad import ArchicadError, batched, run_tapir
from utilities.export import FILE_TYPES, write_table
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Sheet Index Export"

COLUMNS = [
    ("subset", "Subset", 150, "w"),
    ("number", "Number", 80, "w"),
    ("name", "Name", 220, "w"),
    ("size", "Size", 150, "w"),
    ("revision", "Revision", 80, "w"),
    ("status", "Status", 70, "w"),
    ("changes", "Changes", 280, "w"),
]

# ISO 216 A and B sizes in mm (short side, long side), to name the paper size of a layout
PAPER_SIZES = [
    ("A0", 841, 1189), ("A1", 594, 841), ("A2", 420, 594), ("A3", 297, 420), ("A4", 210, 297), ("A5", 148, 210),
    ("B0", 1000, 1414), ("B1", 707, 1000), ("B2", 500, 707), ("B3", 353, 500), ("B4", 250, 353),
]


def label(item: dict) -> str:
    """ The ID and name of a navigator item as the Layout Book shows them. """
    return " ".join(part for part in (item.get("uiId", ""), item.get("name", "")) if part)


def paper_size(width: float | None, height: float | None) -> str:
    if not width or not height:
        return ""
    w, h = round(width), round(height)
    text = f"{w} × {h} mm"
    short, long = sorted((w, h))
    for name, size_short, size_long in PAPER_SIZES:
        if abs(short - size_short) <= 1 and abs(long - size_long) <= 1:
            return f"{name} {'landscape' if w > h else 'portrait'} ({text})"
    return text


def collect_layouts(item: dict, subsets: list[str], layouts: list[dict]) -> None:
    """ The layouts below item in Layout Book order, with the path of the subsets they are in.
    The master layouts are left out. """
    for child_wrapper in item.get("children", []):
        child = child_wrapper["navigatorItem"]
        item_type = child.get("type")
        if item_type == "LayoutItem":
            layouts.append({"navigatorItemId": child["navigatorItemId"], "subset": " / ".join(subsets),
                            "number": child.get("uiId", ""), "name": child.get("name", "")})
        elif item_type == "SubSetItem":
            collect_layouts(child, subsets + [label(child)], layouts)
        elif item_type not in ("MasterFolderItem", "MasterLayoutItem"):
            collect_layouts(child, subsets, layouts)


class SheetIndex:
    """ The Archicad side: reading the layouts and their revisions. """

    def __init__(self):
        self.layouts: list[dict] = []
        self.warnings: list[str] = []

    def read(self) -> None:
        self.warnings = []
        tree = run_tapir("GetNavigatorItemTree", {"navigatorMapId": "LayoutBook"})["navigatorItemTree"]
        layouts: list[dict] = []
        collect_layouts(tree, [], layouts)
        self._read_settings(layouts)
        self._read_database_ids(layouts)
        self._read_revisions(layouts)
        self.layouts = layouts

    def _read_settings(self, layouts: list[dict]) -> None:
        for chunk in batched(layouts):
            settings = run_tapir("GetLayoutSettings", {
                "layoutDatabaseIds": [{"navigatorItemId": layout["navigatorItemId"]} for layout in chunk]
            }).get("layoutSettings", [])
            for layout, setting in zip(chunk, settings):
                if "error" in setting:
                    self.warnings.append(f"{layout['number']} {layout['name']}: no layout settings")
                    continue
                layout["size"] = paper_size(setting.get("horizontalSize"), setting.get("verticalSize"))
                if not layout["number"] and setting.get("customLayoutNumbering"):
                    layout["number"] = setting.get("customLayoutNumber", "")

    def _read_database_ids(self, layouts: list[dict]) -> None:
        for chunk in batched(layouts):
            databases = run_tapir("GetDatabaseIdFromNavigatorItemId", {
                "navigatorItemIds": [{"navigatorItemId": layout["navigatorItemId"]} for layout in chunk]
            }).get("databases", [])
            for layout, database in zip(chunk, databases):
                if "databaseId" in database:
                    layout["databaseId"] = database["databaseId"]
                else:
                    self.warnings.append(f"{layout['number']} {layout['name']}: no database, so no revision")

    def _read_revisions(self, layouts: list[dict]) -> None:
        """ The current revision of a layout is its Actual (not yet issued) revision, or the
        last issued one when it has none. Without Revision Management these stay empty. """
        try:
            revisions = run_tapir("GetDocumentRevisions").get("documentRevisions", [])
        except ArchicadError as e:
            self.warnings.append(f"No revisions: {e}")
            revisions = []
        current: dict[str, dict] = {}
        for revision in revisions:
            guid = revision.get("layoutInfo", {}).get("databaseId", {}).get("guid")
            if guid and (guid not in current or current[guid].get("status") != "Actual"):
                current[guid] = revision

        # The changes are those of the revision shown, so their descriptions are looked up by ID
        descriptions: dict[str, str] = {}
        if any(revision.get("changes") for revision in current.values()):
            try:
                for change in run_tapir("GetRevisionChanges").get("revisionChanges", []):
                    descriptions[change.get("id", "")] = change.get("description", "")
            except ArchicadError as e:
                self.warnings.append(f"No descriptions of the revision changes: {e}")

        for layout in layouts:
            if "databaseId" not in layout:
                continue
            revision = current.get(layout["databaseId"].get("guid"))
            if revision:
                layout["revision"] = revision.get("finalId") or revision.get("id", "")
                layout["status"] = revision.get("status", "")
                change_ids = [change.get("id", "") for change in revision.get("changes", [])]
                layout["changes"] = "; ".join(
                    " ".join(part for part in (change_id, descriptions.get(change_id, "")) if part)
                    for change_id in change_ids if change_id)

    def rows(self) -> list[list[str]]:
        return [[layout.get(key, "") for key, *_ in COLUMNS] for layout in self.layouts]


class SheetIndexWindow:
    """ The window: the sheet index as a table, with saving and copying. """

    def __init__(self):
        self.index = SheetIndex()
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists every layout of the Layout Book with its subset, number, name, size, current revision "
            "and the changes of that revision. Save the list as Excel or CSV, or copy it to paste into a "
            "spreadsheet. Nothing in the project is changed.",
            min_width=760, min_height=460)
        body = self.window.body
        body.rowconfigure(0, weight=1)
        self.tree = make_tree(body, COLUMNS, height=16)
        self.tree.container.grid(row=0, column=0, sticky="nsew")
        self.window.add_button("Read again", self.read)
        self.window.add_button("Copy to clipboard", self.copy)
        self.window.add_button("Save as...", self.save, primary=True)

    def read(self) -> None:
        with self.window.busy("Reading the Layout Book and the revisions..."):
            self.index.read()
            self.tree.delete(*self.tree.get_children())
            for row in self.index.rows():
                self.tree.insert("", "end", values=row)
        for warning in self.index.warnings:
            print(warning)
        count = len(self.index.layouts)
        if not count:
            self.window.set_status("The Layout Book has no layouts.", "warning")
        elif self.index.warnings:
            self.window.set_status(f"{count} layout(s). {len(self.index.warnings)} problem(s), "
                                   f"the first: {self.index.warnings[0]}", "warning")
        else:
            self.window.set_status(f"{count} layout(s).", "ok")

    def copy(self) -> None:
        if not self.index.layouts:
            self.window.set_status("There are no layouts to copy.", "warning")
            return
        def cell(value) -> str:
            return str(value).replace("\t", " ").replace("\r", " ").replace("\n", " ")
        lines = ["\t".join(heading for _, heading, *_ in COLUMNS)]
        lines += ["\t".join(cell(value) for value in row) for row in self.index.rows()]
        self.window.root.clipboard_clear()
        self.window.root.clipboard_append("\n".join(lines))
        self.window.root.update()
        self.window.set_status(f"Copied {len(self.index.layouts)} layout(s). Paste them into a spreadsheet.", "ok")

    def save(self) -> None:
        if not self.index.layouts:
            self.window.set_status("There are no layouts to save.", "warning")
            return
        stamp = datetime.datetime.now().strftime("%Y-%m-%d")
        path = filedialog.asksaveasfilename(parent=self.window.root, title="Save the sheet index",
                                            initialfile=f"Sheet index {stamp}.xlsx", defaultextension=".xlsx",
                                            filetypes=FILE_TYPES)
        if not path:
            return
        write_table(path, [heading for _, heading, *_ in COLUMNS], self.index.rows(), "Sheet index")
        self.window.set_status(f"Saved {len(self.index.layouts)} layout(s) to {os.path.basename(path)}.", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.read)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    SheetIndexWindow().run()
