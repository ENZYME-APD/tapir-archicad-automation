# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Bulk View Renamer

Lists the views of the View Map, or of one of its folders, and renames them in
one go: find and replace (optionally a regular expression), a prefix and a
suffix, or a pattern with a counter. A table shows the old and the new name of
every view with name clashes marked, and nothing is renamed until you confirm.
The views are renamed one by one with RenameNavigatorItem, and every view that
could not be renamed is reported.
"""

import re
import tkinter as tk
from dataclasses import dataclass
from tkinter import ttk

from utilities.archicad import ArchicadError, run_command, run_tapir
from utilities.navigator_utils import VIEW_NAVITEM_TYPES, NavigatorItem
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Bulk View Renamer"
WHOLE_VIEW_MAP = "(whole View Map)"
FOLDER_TYPES = ("FolderItem", "UndefinedItem")  # UndefinedItem: clone folders
PATH_SEPARATOR = " / "
PREVIEW_LINES = 15
MODES = [("replace", "Find and replace"), ("affix", "Prefix and suffix"), ("pattern", "Pattern with counter")]
PATTERN_TOKEN = re.compile(r"\{(name|id|n)\}")

STATUS_RENAME, STATUS_UNCHANGED, STATUS_EMPTY, STATUS_CLASH, STATUS_KEPT = (
    "rename", "unchanged", "empty - skipped", "clash", "not selected")


@dataclass
class View:
    guid: str
    view_id: str
    name: str
    folder: str  # the folder path, "" directly under the View Map
    parent: str  # the guid of the folder that holds the view
    new_name: str = ""
    status: str = STATUS_UNCHANGED


def read_views() -> tuple[list[View], list[str]]:
    """ The views of the View Map in the order the View Map shows them, and the folder paths. """
    tree = run_command("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
    views: list[View] = []
    folders: list[str] = []

    def walk(item: NavigatorItem, path: list[str]) -> None:
        for child_wrapper in item.get("children", []):
            child = child_wrapper["navigatorItem"]
            item_type = child["type"]
            if item_type in VIEW_NAVITEM_TYPES:
                views.append(View(guid=child["navigatorItemId"]["guid"], view_id=child.get("prefix", ""),
                                  name=child.get("name", ""), folder=PATH_SEPARATOR.join(path),
                                  parent=item["navigatorItemId"]["guid"]))
            child_path = path
            if item_type in FOLDER_TYPES:
                child_path = path + [child.get("name", "") or "(unnamed)"]
                folders.append(PATH_SEPARATOR.join(child_path))
            walk(child, child_path)

    walk(tree["navigatorTree"]["rootItem"], [])
    return views, folders


def in_folder(view: View, folder: str) -> bool:
    return folder == WHOLE_VIEW_MAP or view.folder == folder or view.folder.startswith(folder + PATH_SEPARATOR)


class BulkViewRenamer:
    """ The window: picks the views, previews the new names and renames them. """

    def __init__(self):
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Renames the views of the View Map, or of one folder, in one go. Choose how the names change: the table "
            "shows the new names and marks the clashes (two views in the same folder with the same name). Select rows "
            "to rename only those, otherwise every listed view is renamed. Nothing changes until you confirm.",
            min_width=760, min_height=600)
        self.views: list[View] = []
        self.listed: list[View] = []
        self._build_body()
        self.window.add_button("Read again", self.read)
        self.rename_button = self.window.add_button("Rename...", self.rename, primary=True)
        self.rename_button.state(["disabled"])

    # ---- layout ----

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(3, weight=1)

        scope = ttk.Frame(body)
        scope.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        scope.columnconfigure(1, weight=1)
        ttk.Label(scope, text="Views in:").grid(row=0, column=0, sticky="w")
        self.folder = tk.StringVar(value=WHOLE_VIEW_MAP)
        self.folder_box = ttk.Combobox(scope, textvariable=self.folder, state="readonly", values=[WHOLE_VIEW_MAP])
        self.folder_box.grid(row=0, column=1, sticky="ew", padx=(6, 0))
        self.folder_box.bind("<<ComboboxSelected>>", lambda _e: self._fill_table())

        self.mode = tk.StringVar(value=MODES[0][0])
        modes = ttk.Frame(body)
        modes.grid(row=1, column=0, sticky="ew")
        for column, (key, caption) in enumerate(MODES):
            ttk.Radiobutton(modes, text=caption, value=key, variable=self.mode,
                            command=self._show_mode).grid(row=0, column=column, sticky="w", padx=(0, 16))

        self.options = ttk.Frame(body, padding=(0, 6, 0, 8))
        self.options.grid(row=2, column=0, sticky="ew")
        self.vars: dict[str, tk.Variable] = {
            "find": tk.StringVar(), "replace": tk.StringVar(),
            "case": tk.BooleanVar(value=False), "regex": tk.BooleanVar(value=False),
            "prefix": tk.StringVar(), "suffix": tk.StringVar(),
            "pattern": tk.StringVar(value="{name} {n}"), "start": tk.StringVar(value="1"),
            "step": tk.StringVar(value="1"), "digits": tk.StringVar(value="2"),
        }
        self.mode_frames = {
            "replace": self._mode_frame([("Find:", "find", 24), ("Replace with:", "replace", 24)],
                                        [("Match case", "case"), ("Regular expression", "regex")]),
            "affix": self._mode_frame([("Add before:", "prefix", 20), ("Add after:", "suffix", 20)], []),
            "pattern": self._mode_frame([("Pattern:", "pattern", 24), ("Start:", "start", 5), ("Step:", "step", 5),
                                         ("Digits:", "digits", 4)], []),
        }
        ttk.Label(self.mode_frames["pattern"], style="Muted.TLabel",
                  text="{name} is the current name, {id} the view's ID, {n} the counter "
                       "(counting in View Map order over the listed views, or the selected ones).").grid(
            row=1, column=0, columnspan=8, sticky="w", pady=(4, 0))
        for variable in self.vars.values():
            variable.trace_add("write", lambda *_args: self._update_preview())

        self.tree = make_tree(body, [("folder", "Folder", 170, "w"), ("id", "ID", 70, "w"),
                                     ("old", "Current name", 190, "w"), ("new", "New name", 190, "w"),
                                     ("status", "Status", 100, "w")], height=14)
        for column in ("old", "new"):
            self.tree.column(column, stretch=True)
        self.tree.container.grid(row=3, column=0, sticky="nsew")
        self.tree.tag_configure(STATUS_CLASH, foreground="#c62828")
        self.tree.tag_configure(STATUS_EMPTY, foreground="#a15c00")
        self.tree.tag_configure(STATUS_UNCHANGED, foreground="#999999")
        self.tree.tag_configure(STATUS_KEPT, foreground="#999999")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self._update_preview())

        selection = ttk.Frame(body, padding=(0, 6, 0, 0))
        selection.grid(row=4, column=0, sticky="ew")
        ttk.Button(selection, text="Clear selection (rename every listed view)",
                   command=lambda: self.tree.selection_remove(*self.tree.selection())).grid(row=0, column=0)

        self._show_mode()

    def _mode_frame(self, entries: list[tuple[str, str, int]], checks: list[tuple[str, str]]) -> ttk.Frame:
        frame = ttk.Frame(self.options)
        column = 0
        for caption, key, width in entries:
            ttk.Label(frame, text=caption).grid(row=0, column=column, sticky="w", padx=(0 if column == 0 else 12, 4))
            ttk.Entry(frame, textvariable=self.vars[key], width=width).grid(row=0, column=column + 1, sticky="w")
            column += 2
        for caption, key in checks:
            ttk.Checkbutton(frame, text=caption, variable=self.vars[key]).grid(row=0, column=column, sticky="w",
                                                                              padx=(12, 0))
            column += 1
        return frame

    def _show_mode(self) -> None:
        for key, frame in self.mode_frames.items():
            if key == self.mode.get():
                frame.grid(row=0, column=0, sticky="w")
            else:
                frame.grid_remove()
        self._update_preview()

    # ---- the new names ----

    def _int(self, key: str, default: int) -> int:
        try:
            return int(self.vars[key].get().strip())
        except ValueError:
            return default

    def _renamer(self):
        """ A function giving the new name of a view and its counter value, or raising ValueError. """
        mode = self.mode.get()
        if mode == "replace":
            find = self.vars["find"].get()
            if not find:
                return lambda view, _n: view.name
            flags = 0 if self.vars["case"].get() else re.IGNORECASE
            try:
                expression = re.compile(find if self.vars["regex"].get() else re.escape(find), flags)
            except re.error as e:
                raise ValueError(f"The regular expression is not valid: {e}") from e
            replacement = self.vars["replace"].get()
            if self.vars["regex"].get():
                def regex_replace(view: View, _n: int) -> str:
                    try:
                        return expression.sub(replacement, view.name)
                    except (re.error, IndexError) as e:  # e.g. \2 with one group
                        raise ValueError(f"The replacement is not valid: {e}") from e
                return regex_replace
            return lambda view, _n: expression.sub(lambda _m: replacement, view.name)
        if mode == "affix":
            prefix, suffix = self.vars["prefix"].get(), self.vars["suffix"].get()
            return lambda view, _n: f"{prefix}{view.name}{suffix}"
        pattern = self.vars["pattern"].get()
        digits = max(0, min(self._int("digits", 0), 10))

        def from_pattern(view: View, n: int) -> str:
            values = {"name": view.name, "id": view.view_id, "n": str(n).zfill(digits) if n >= 0 else str(n)}
            return PATTERN_TOKEN.sub(lambda m: values[m.group(1)], pattern)
        return from_pattern

    def _update_preview(self) -> None:
        if not hasattr(self, "rename_button"):  # still building the window
            return
        selected = set(self.tree.selection())
        try:
            self._compute_new_names(selected)
        except ValueError as e:
            for view in self.listed:
                view.new_name, view.status = view.name, STATUS_UNCHANGED
            self._show_rows()
            self.rename_button.state(["disabled"])
            self.window.set_status(str(e), "error")
            return
        self._mark_clashes()
        self._show_rows()

        to_rename = self._to_rename()
        clashes = sum(1 for view in self.listed if view.status == STATUS_CLASH)
        empty = sum(1 for view in self.listed if view.status == STATUS_EMPTY)
        self.rename_button.state(["!disabled"] if to_rename else ["disabled"])
        notes = []
        if clashes:
            notes.append(f"{clashes} clash(es)")
        if empty:
            notes.append(f"{empty} empty name(s) skipped")
        scope = "selected" if selected else "listed"
        text = f"{len(to_rename)} of the {len(selected) if selected else len(self.listed)} {scope} view(s) get a new name."
        if notes:
            text += " " + ", ".join(notes) + "."
        self.window.set_status(text, "warning" if clashes or empty else "info")

    def _compute_new_names(self, selected: set[str]) -> None:
        """ Sets the new name and the status of every listed view; raises ValueError on bad settings. """
        renamer = self._renamer()
        counter, step = self._int("start", 1), self._int("step", 1)
        for view in self.listed:
            if selected and view.guid not in selected:
                view.new_name, view.status = view.name, STATUS_KEPT
                continue
            view.new_name = renamer(view, counter).strip()
            counter += step
            if not view.new_name:
                view.status = STATUS_EMPTY
            elif view.new_name == view.name:
                view.status = STATUS_UNCHANGED
            else:
                view.status = STATUS_RENAME

    def _mark_clashes(self) -> None:
        """ A renamed view clashes when another view in the same folder ends up with the same name.
        Every view of the folder counts, also those outside the scope or not selected. """
        listed = {view.guid for view in self.listed}
        final_names: dict[tuple[str, str], int] = {}
        for view in self.views:
            name = view.new_name if view.guid in listed and view.status == STATUS_RENAME else view.name
            key = (view.parent, name.casefold())
            final_names[key] = final_names.get(key, 0) + 1
        for view in self.listed:
            if view.status == STATUS_RENAME and final_names[(view.parent, view.new_name.casefold())] > 1:
                view.status = STATUS_CLASH

    def _to_rename(self) -> list[View]:
        return [view for view in self.listed if view.status in (STATUS_RENAME, STATUS_CLASH)]

    def _show_rows(self) -> None:
        for view in self.listed:
            new_name = view.new_name if view.status in (STATUS_RENAME, STATUS_CLASH, STATUS_EMPTY) else ""
            self.tree.item(view.guid, values=(view.folder or "-", view.view_id, view.name, new_name, view.status),
                           tags=(view.status,))

    def _fill_table(self) -> None:
        self.tree.delete(*self.tree.get_children())
        folder = self.folder.get()
        self.listed = [view for view in self.views if in_folder(view, folder)]
        for view in self.listed:
            # a view can be listed only once: the guids of the View Map items are unique
            self.tree.insert("", "end", iid=view.guid, values=(view.folder or "-", view.view_id, view.name, "", ""))
        self._update_preview()

    # ---- actions ----

    def read(self) -> None:
        with self.window.busy("Reading the View Map..."):
            self.views, folders = read_views()
        self.folder_box.configure(values=[WHOLE_VIEW_MAP] + folders)
        if self.folder.get() not in folders:
            self.folder.set(WHOLE_VIEW_MAP)
        self._fill_table()
        if not self.views:
            self.window.set_status("The View Map has no views.", "warning")

    def rename(self) -> None:
        views = self._to_rename()
        if not views:
            return
        clashes = sum(1 for view in views if view.status == STATUS_CLASH)
        lines = [f"{view.name}  →  {view.new_name}" for view in views[:PREVIEW_LINES]]
        if len(views) > PREVIEW_LINES:
            lines.append(f"... and {len(views) - PREVIEW_LINES} more")
        message = f"Rename {len(views)} view(s)?\n\n" + "\n".join(lines)
        if clashes:
            message += (f"\n\n{clashes} of them will have the same name as another view in their folder "
                        "(marked as clash in the table).")
        if not self.window.ask_ok_cancel(SCRIPT_NAME, message):
            return

        failures: list[tuple[View, str]] = []
        try:
            with self.window.busy(f"Renaming {len(views)} view(s)..."):
                for view in views:
                    try:
                        run_tapir("RenameNavigatorItem", {"navigatorItemId": {"guid": view.guid},
                                                          "newName": view.new_name})
                    except ArchicadError as e:
                        failures.append((view, str(e)))
        finally:
            # the table shows the names Archicad has now, also after a failure part way
            self.read()
        renamed = len(views) - len(failures)
        if failures:
            print(f"{len(failures)} view(s) could not be renamed:")
            for view, error in failures:
                print(f"  {view.folder}{PATH_SEPARATOR if view.folder else ''}{view.view_id} {view.name} "
                      f"-> {view.new_name}: {error}")
            first_view, first_error = failures[0]
            self.window.set_status(f"Renamed {renamed} view(s), {len(failures)} failed (see the palette output). "
                                   f"First: {first_view.name}: {first_error}", "error")
        else:
            self.window.set_status(f"Renamed {renamed} view(s).", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.read)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    BulkViewRenamer().run()
