# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///

"""
Unused View Cleaner

Finds the views of the View Map that are neither placed on a layout nor
published by a Publisher Set, and moves the chosen ones into a separate folder
that repeats their original folder structure. A clone folder is only moved as
a whole, when none of its views is used.
"""

import tkinter as tk
from tkinter import ttk
from typing import Callable

from utilities.archicad import run_command
from utilities.navigator_utils import (VIEW_NAVITEM_TYPES, Guid, NavigatorItem, NavigatorItemId, NavigatorItemWrapper,
                                       NavigatorTreeIdWrapper, get_path_to_navitem, get_unique_navigator_items_from_tree,
                                       merge_paths)
from utilities.ui import ScriptWindow, make_tree, require_archicad

SCRIPT_NAME = "Unused View Cleaner"
DEFAULT_FOLDER_NAME = "Unused Views"
CHECKED, UNCHECKED, PARTIAL, LOCKED = "☑", "☐", "◩", "–"


class UnusedViewCleaner:
    """ The Archicad side: finding the unused views and moving them. """

    def __init__(self):
        self.moved_views = 0
        self.node_processors: dict[str, Callable[[Guid, NavigatorItemWrapper], Guid | None]] = {
            "FolderItem": self.process_folder_node,
            "UndefinedItem": self.process_undefined_node,
            **{item: self.process_view_node for item in VIEW_NAVITEM_TYPES},
        }
        self.all_views: list[NavigatorItemId] = []
        self.on_layouts: set[str] = set()
        self.in_publisher_sets: set[str] = set()
        self.paths: dict[str, list[NavigatorItemWrapper]] = {}

    # ---- analysis ----

    def analyze(self, skip_folder_name: str) -> None:
        self.on_layouts = self._source_guids(["DrawingItem"], [{"navigatorTreeId": {"type": "LayoutBook"}}])
        publisher_set_names = run_command("API.GetPublisherSetNames")["publisherSetNames"]
        self.in_publisher_sets = self._source_guids(
            VIEW_NAVITEM_TYPES, [{"navigatorTreeId": {"type": "PublisherSets", "name": name}} for name in publisher_set_names])
        self.all_views = [{"navigatorItemId": view["navigatorItemId"]}
                          for view in self._navigator_items(VIEW_NAVITEM_TYPES, [{"navigatorTreeId": {"type": "ViewMap"}}])]

        used = self.on_layouts | self.in_publisher_sets
        view_map_tree = run_command("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
        main_branch = view_map_tree["navigatorTree"]["rootItem"].get("children", [])
        self.paths = {}
        if not main_branch:
            return
        for view in self.all_views:
            guid = view["navigatorItemId"]["guid"]
            if guid in used:
                continue
            path = get_path_to_navitem(main_branch[0], view)
            # views already moved into the target folder by an earlier run are not listed again
            if len(path) > 2 and path[1]["navigatorItem"]["type"] == "FolderItem" \
                    and path[1]["navigatorItem"]["name"] == skip_folder_name:
                continue
            if path:
                self.paths[guid] = path

    def _navigator_items(self, navitem_types: list[str], tree_ids: list[NavigatorTreeIdWrapper]) -> list[NavigatorItem]:
        guid_navitem_map: dict[str, NavigatorItem] = {}
        for tree_id in tree_ids:
            tree = run_command("API.GetNavigatorItemTree", tree_id)
            get_unique_navigator_items_from_tree(
                current_branch=tree["navigatorTree"]["rootItem"].get("children", []),
                guid_navitem_map=guid_navitem_map,
                navitem_types=navitem_types,
            )
        return list(guid_navitem_map.values())

    def _source_guids(self, navitem_types: list[str], tree_ids: list[NavigatorTreeIdWrapper]) -> set[str]:
        return {item["sourceNavigatorItemId"]["guid"] for item in self._navigator_items(navitem_types, tree_ids)
                if "sourceNavigatorItemId" in item}

    def unused_tree(self, guids: list[str] | None = None) -> NavigatorItemWrapper | dict:
        selected = self.paths.keys() if guids is None else guids
        return merge_paths([self.paths[guid] for guid in selected if guid in self.paths])

    def clone_folder_source_count(self, clone_folder: NavigatorItem) -> int:
        result = run_command("API.GetBuiltInContainerNavigatorItems", {
            "navigatorItemIds": [{"navigatorItemId": clone_folder["sourceNavigatorItemId"]}]})
        return len(result["navigatorItems"][0]["builtInContainerNavigatorItem"]["contentIds"])

    # ---- moving ----

    def move(self, guids: list[str], folder_name: str) -> int:
        self.moved_views = 0
        tree = self.unused_tree(guids)
        if not tree:
            return 0
        self._index_existing_folders()
        root_for_unused_views = self._folder(tree["navigatorItem"]["navigatorItemId"], folder_name)
        for child in tree["navigatorItem"].get("children", []):
            self.process_unused_tree(root_for_unused_views, child)
        return self.moved_views

    def _index_existing_folders(self) -> None:
        """ The folders already in the View Map, so a later run moves views into
        them instead of creating a second folder with the same name. """
        self.existing_folders: dict[str, dict[str, Guid]] = {}

        def walk(item: NavigatorItem) -> None:
            children = [c["navigatorItem"] for c in item.get("children", [])]
            self.existing_folders[item["navigatorItemId"]["guid"]] = {
                c["name"]: c["navigatorItemId"] for c in children if c["type"] == "FolderItem"}
            for child in children:
                walk(child)

        view_map_tree = run_command("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
        walk(view_map_tree["navigatorTree"]["rootItem"])

    def _folder(self, parent_guid: Guid, name: str) -> Guid:
        """ The folder called name under parent, created when it does not exist yet. """
        siblings = self.existing_folders.setdefault(parent_guid["guid"], {})
        if name not in siblings:
            created = run_command("API.CreateViewMapFolder", {
                "folderParameters": {"name": name},
                "parentNavigatorItemId": parent_guid,
            })
            siblings[name] = created["createdFolderNavigatorItemId"]
            self.existing_folders[siblings[name]["guid"]] = {}
        return siblings[name]

    def process_unused_tree(self, parent_guid: Guid, current_branch: NavigatorItemWrapper) -> None:
        node_processor = self.node_processors[current_branch["navigatorItem"]["type"]]
        new_parent_guid = node_processor(parent_guid, current_branch)
        children = current_branch["navigatorItem"].get("children")
        if children and new_parent_guid:
            for child in children:
                self.process_unused_tree(new_parent_guid, child)

    def process_folder_node(self, parent_guid: Guid, folder_node: NavigatorItemWrapper) -> Guid:
        return self._folder(parent_guid, folder_node["navigatorItem"]["name"])

    def process_view_node(self, parent_guid: Guid, view_node: NavigatorItemWrapper) -> None:
        run_command("API.MoveNavigatorItem", {
            "navigatorItemIdToMove": view_node["navigatorItem"]["navigatorItemId"],
            "parentNavigatorItemId": parent_guid,
        })
        self.moved_views += 1
        return None

    def process_undefined_node(self, parent_guid: Guid, clone_folder_node: NavigatorItemWrapper) -> None:
        """ Clone folders: their views cannot be moved one by one, so the folder is
        only moved when all of its views are unused. """
        clone_folder = clone_folder_node["navigatorItem"]
        unused_count = len(clone_folder.get("children", []))
        if self.clone_folder_source_count(clone_folder) == unused_count:
            run_command("API.MoveNavigatorItem", {
                "navigatorItemIdToMove": clone_folder["navigatorItemId"],
                "parentNavigatorItemId": parent_guid,
            })
            self.moved_views += unused_count
        return None


class UnusedViewCleanerWindow:
    """ The window: shows what is unused and moves what the user keeps checked. """

    def __init__(self):
        self.cleaner = UnusedViewCleaner()
        self.window = ScriptWindow(
            SCRIPT_NAME,
            "Lists the views of the View Map that are neither placed on a layout nor published by a Publisher Set. "
            "Click a line to include or exclude it, then move the checked views into a separate folder. "
            "Nothing is moved until you press Move.",
            min_width=600, min_height=520)
        # tree item id -> the view guids it stands for; leaves are views or whole clone folders
        self.leaf_views: dict[str, list[str]] = {}
        self.checked: dict[str, bool] = {}
        self.locked: set[str] = set()
        self._build_body()
        self.window.add_button("Analyze again", self.analyze)
        self.move_button = self.window.add_button("Move checked views", self.move, primary=True)
        self.move_button.state(["disabled"])

    def _build_body(self) -> None:
        body = self.window.body
        body.rowconfigure(1, weight=1)

        stats = ttk.Frame(body)
        stats.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        self.stat_labels = {}
        for column, (key, caption) in enumerate([("all", "views in the View Map"), ("layouts", "placed on layouts"),
                                                 ("publisher", "in Publisher Sets"), ("unused", "unused")]):
            stats.columnconfigure(column, weight=1)
            value = ttk.Label(stats, text="–", style="Big.TLabel")
            value.grid(row=0, column=column, sticky="w")
            ttk.Label(stats, text=caption, style="Muted.TLabel").grid(row=1, column=column, sticky="w")
            self.stat_labels[key] = value

        self.tree = make_tree(body, [("check", "Move", 56, "center"), ("kind", "Kind", 190, "w")], height=12, show_tree=True)
        self.tree.heading("#0", text="View", anchor="w")
        self.tree.column("#0", width=300, stretch=True)
        self.tree.container.grid(row=1, column=0, sticky="nsew")
        self.tree.tag_configure("locked", foreground="#999999")
        self.tree.bind("<ButtonRelease-1>", self._on_click)
        self.tree.bind("<space>", lambda _e: self._toggle(self.tree.focus()))

        target = ttk.Frame(body, padding=(0, 8, 0, 0))
        target.grid(row=2, column=0, sticky="ew")
        ttk.Label(target, text="Move into the folder:").grid(row=0, column=0, sticky="w")
        self.folder_name = tk.StringVar(value=DEFAULT_FOLDER_NAME)
        ttk.Entry(target, textvariable=self.folder_name, width=28).grid(row=0, column=1, sticky="w", padx=(6, 0))
        ttk.Button(target, text="Check all", command=lambda: self._set_all(True)).grid(row=0, column=2, padx=(16, 0))
        ttk.Button(target, text="Uncheck all", command=lambda: self._set_all(False)).grid(row=0, column=3, padx=(6, 0))

    # ---- tree ----

    def _insert(self, parent: str, wrapper: NavigatorItemWrapper) -> None:
        item = wrapper["navigatorItem"]
        iid = item["navigatorItemId"]["guid"]
        label = " ".join(part for part in (item.get("prefix", ""), item.get("name", "")) if part) or "(unnamed)"
        item_type = item["type"]
        if item_type == "FolderItem":
            self.tree.insert(parent, "end", iid=iid, text=label, values=("", "Folder"), open=True)
            for child in item.get("children", []):
                self._insert(iid, child)
        elif item_type == "UndefinedItem":
            unused = [c["navigatorItem"]["navigatorItemId"]["guid"] for c in item.get("children", [])]
            total = self.cleaner.clone_folder_source_count(item)
            movable = total == len(unused)
            kind = f"Clone folder, {len(unused)} views" if movable else "Clone folder, partly used"
            self.tree.insert(parent, "end", iid=iid, text=label, values=("", kind), tags=() if movable else ("locked",))
            self.leaf_views[iid] = unused
            if movable:
                self.checked[iid] = True
            else:
                self.locked.add(iid)
        else:
            self.tree.insert(parent, "end", iid=iid, text=label, values=("", item_type.removesuffix("Item")))
            self.leaf_views[iid] = [iid]
            self.checked[iid] = True

    def _leaves_under(self, iid: str) -> list[str]:
        if iid in self.leaf_views:
            return [] if iid in self.locked else [iid]
        leaves = []
        for child in self.tree.get_children(iid):
            leaves += self._leaves_under(child)
        return leaves

    def _refresh_marks(self, iid: str = "") -> str:
        """ Updates the check marks below iid and returns the mark of iid itself. """
        if iid and iid in self.locked:
            mark = LOCKED
        elif iid in self.leaf_views:
            mark = CHECKED if self.checked.get(iid) else UNCHECKED
        else:
            marks = {self._refresh_marks(child) for child in self.tree.get_children(iid)} - {LOCKED}
            mark = marks.pop() if len(marks) == 1 else (UNCHECKED if not marks else PARTIAL)
        if iid:
            self.tree.set(iid, "check", mark)
        return mark

    def _toggle(self, iid: str) -> None:
        if not iid:
            return
        leaves = self._leaves_under(iid)
        if not leaves:
            return
        new_state = not all(self.checked[leaf] for leaf in leaves)
        for leaf in leaves:
            self.checked[leaf] = new_state
        self._update_counts()

    def _on_click(self, event) -> None:
        if self.tree.identify_region(event.x, event.y) in ("cell", "tree") and \
                self.tree.identify_element(event.x, event.y) != "Treeitem.indicator":
            self._toggle(self.tree.identify_row(event.y))

    def _set_all(self, state: bool) -> None:
        for leaf in self.checked:
            self.checked[leaf] = state
        self._update_counts()

    def _checked_views(self) -> list[str]:
        return [guid for leaf, on in self.checked.items() if on for guid in self.leaf_views[leaf]]

    def _update_counts(self) -> None:
        self._refresh_marks()
        count = len(self._checked_views())
        self.move_button.state(["!disabled"] if count else ["disabled"])
        self.window.set_status(f"{count} view(s) checked to move." if count else "Nothing is checked to move.")

    # ---- actions ----

    def analyze(self) -> None:
        with self.window.busy("Reading the View Map, the Layout Book and the Publisher Sets..."):
            self.cleaner.analyze(skip_folder_name=self.folder_name.get().strip() or DEFAULT_FOLDER_NAME)
            self.tree.delete(*self.tree.get_children())
            self.leaf_views, self.checked, self.locked = {}, {}, set()
            tree = self.cleaner.unused_tree()
            for child in (tree["navigatorItem"].get("children", []) if tree else []):
                self._insert("", child)

        self.stat_labels["all"].configure(text=str(len(self.cleaner.all_views)))
        self.stat_labels["layouts"].configure(text=str(len(self.cleaner.on_layouts)))
        self.stat_labels["publisher"].configure(text=str(len(self.cleaner.in_publisher_sets)))
        unused = len(self.cleaner.paths)
        self.stat_labels["unused"].configure(text=str(unused), foreground="#a15c00" if unused else "#1b7f3b")
        if unused:
            self._update_counts()
        else:
            self.move_button.state(["disabled"])
            self.window.set_status("Every view is used - there is nothing to clean up.", "ok")

    def move(self) -> None:
        guids = self._checked_views()
        folder_name = self.folder_name.get().strip() or DEFAULT_FOLDER_NAME
        if not guids or not self.window.ask_ok_cancel(
                SCRIPT_NAME, f"Move {len(guids)} view(s) into the \"{folder_name}\" folder of the View Map?\n\n"
                             "The folder structure of the moved views is repeated inside it."):
            return
        try:
            with self.window.busy(f"Moving {len(guids)} view(s)..."):
                self.cleaner.move(guids, folder_name)
        finally:
            # the moved views now live under the target folder, which analyze
            # skips, so the list shows only what is still left to clean up -
            # also after a failure part way, when some views were moved already
            moved = self.cleaner.moved_views
            self.analyze()
        self.window.set_status(f"Moved {moved} view(s) into \"{folder_name}\".", "ok")

    def run(self) -> None:
        self.window.run(on_start=self.analyze)


if __name__ == "__main__":
    require_archicad(SCRIPT_NAME)
    UnusedViewCleanerWindow().run()
