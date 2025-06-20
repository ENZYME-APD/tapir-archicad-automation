from typing import Optional,  Callable
from multiconn_archicad import MultiConn, ConnHeader, CoreCommands

from utilities.tk_utils import show_popup
from utilities.navigator_utils import get_unique_navigator_items_from_tree, merge_paths, get_path_to_navitem, Guid, NavigatorItem, NavigatorItemWrapper, NavigatorItemId, NavigatorTreeIdWrapper, VIEW_NAVITEM_TYPES
from utilities.teamwork_utils import TeamworkReserve

class UnusedViewCleaner:
    """
    The script finds unused views, and moves them to a separate "unused views" folder
    with the copy of the original folder tree. Clone folders are only moved if no views are used.
    """
    def __init__(self):
        self.conn_header: Optional[ConnHeader] = None
        self.core: Optional[CoreCommands] = None
        self.moved_views: int = 0
        self.node_processors: dict[str, Callable[[Guid, NavigatorItemWrapper], Guid | None]] = {
            "FolderItem": self.process_folder_node,
            "UndefinedItem": self.process_undefined_node,
            **{item: self.process_view_node for item in VIEW_NAVITEM_TYPES}
        }
        self.used_views: Optional[list[NavigatorItemId]] = None
        self.unused_views: Optional[list[NavigatorItemId]] = None
        self.unused_folder_tree: Optional[NavigatorItemWrapper] = None

    def run(self, conn_header: ConnHeader) -> int:
        self.init_new_run(conn_header)
        self.get_used_views()
        self.get_unused_views()
        if len(self.unused_views) > 0:
            self.get_unused_folder_tree()
            self.move_unused_views()
        return self.moved_views

    def init_new_run(self, conn_header: ConnHeader) -> None:
        self.conn_header = conn_header
        self.core = conn_header.core
        self.moved_views = 0

    def get_used_views(self) -> None:
        self.used_views = self.get_views_placed_on_layouts() + self.get_views_from_publisher_sets()

    def get_views_placed_on_layouts(self) -> list[NavigatorItemId]:
        all_drawings = self.get_navigator_items_from_tree_ids(
            navitem_types=["DrawingItem"],
            navigator_tree_ids=[{"navigatorTreeId": {"type": "LayoutBook"}}]
        )
        return [{"navigatorItemId": drawing['sourceNavigatorItemId']} for drawing in all_drawings]

    def get_views_from_publisher_sets(self) -> list[NavigatorItemId]:
        publisher_set_names = self.core.post_command("API.GetPublisherSetNames")["publisherSetNames"]
        navigator_tree_ids = [{"navigatorTreeId": {"type": "PublisherSets", "name": p_set}} for p_set in publisher_set_names]
        views_from_publisher_sets = self.get_navigator_items_from_tree_ids(
            navitem_types=VIEW_NAVITEM_TYPES,
            navigator_tree_ids=navigator_tree_ids
        )
        return [{"navigatorItemId": view['sourceNavigatorItemId']} for view in views_from_publisher_sets]

    def get_unused_views(self) -> None:
        all_view_nav_ids = self.get_all_views()
        used_view_guids = {nav_id["navigatorItemId"]["guid"] for nav_id in self.used_views}
        self.unused_views = [nav_id for nav_id in all_view_nav_ids if nav_id["navigatorItemId"]["guid"] not in used_view_guids]

    def get_all_views(self) -> list[NavigatorItemId]:
        navigator_tree_ids = [{"navigatorTreeId": {"type": "ViewMap"}}]
        views_from_publisher_sets = self.get_navigator_items_from_tree_ids(
            navitem_types=VIEW_NAVITEM_TYPES,
            navigator_tree_ids=navigator_tree_ids
        )
        return [{"navigatorItemId": view["navigatorItemId"]} for view in views_from_publisher_sets]

    def get_navigator_items_from_tree_ids(self, navitem_types: list[str], navigator_tree_ids: list[NavigatorTreeIdWrapper]) -> list[NavigatorItem]:
        guid_navitem_map = {}
        for navigator_tree_id in navigator_tree_ids:
            navigator_tree = self.core.post_command(
                command="API.GetNavigatorItemTree", parameters=navigator_tree_id
            )
            get_unique_navigator_items_from_tree(
                current_branch=navigator_tree["navigatorTree"]["rootItem"].get("children", {}),
                guid_navitem_map=guid_navitem_map,
                navitem_types=navitem_types,
            )
        return [navitem for navitem in guid_navitem_map.values()]

    def get_unused_folder_tree(self) -> None:
        path_for_each_unused_view_id = self.get_path_for_each_unused_view_id()
        self.unused_folder_tree = merge_paths(path_for_each_unused_view_id)

    def get_path_for_each_unused_view_id(self) -> list[list[NavigatorItemWrapper]]:
        view_map_tree = self.core.post_command(
                command="API.GetNavigatorItemTree", parameters={"navigatorTreeId": {"type": "ViewMap"}})
        main_branch = view_map_tree["navigatorTree"]["rootItem"].get("children", {})
        return [get_path_to_navitem(main_branch[0], navitem_id) for navitem_id in self.unused_views]

    def move_unused_views(self) -> None:
        root_for_unused_views = self.insert_unused_views_folder()
        for child in self.unused_folder_tree["navigatorItem"]["children"]:
            self.process_unused_tree(
                parent_guid=root_for_unused_views,
                current_branch=child
            )

    def insert_unused_views_folder(self) -> Guid:
        new_folder_navitem_id = self.core.post_command(
            command="API.CreateViewMapFolder",
            parameters={"folderParameters": {"name": "Unused Views"},
                        "parentNavigatorItemId": self.unused_folder_tree["navigatorItem"]["navigatorItemId"]
                        }
        )
        return new_folder_navitem_id["createdFolderNavigatorItemId"]

    def process_unused_tree(self, parent_guid: Guid, current_branch: NavigatorItemWrapper):
        node_processor = self.node_processors[current_branch["navigatorItem"]["type"]]
        new_parent_guid = node_processor(parent_guid, current_branch)
        if (children:= current_branch["navigatorItem"].get("children", False)) and new_parent_guid:
            for child in children:
                self.process_unused_tree(parent_guid=new_parent_guid,
                                         current_branch=child)

    def process_folder_node(self, parent_guid: Guid, folder_node: NavigatorItemWrapper) -> Guid:
        new_folder_navitem_id = self.core.post_command(
            command="API.CreateViewMapFolder",
            parameters={"folderParameters": {"name": folder_node["navigatorItem"]["name"]},
                        "parentNavigatorItemId": parent_guid
                        }
        )
        return new_folder_navitem_id["createdFolderNavigatorItemId"]

    def process_view_node(self, parent_guid: Guid, view_node: NavigatorItemWrapper) -> None:
        with TeamworkReserve(self.conn_header, [view_node["navigatorItem"]["navigatorItemId"]]):
            self.core.post_command(
                command="API.MoveNavigatorItem",
                parameters={"navigatorItemIdToMove": view_node["navigatorItem"]["navigatorItemId"],
                            "parentNavigatorItemId": parent_guid}
            )
        self.moved_views += 1
        return None

    def process_undefined_node(self, parent_guid: Guid, undefined_node: NavigatorItemWrapper) -> None:
        """ These are clone folders. Clone folders children cannot be moved on their own,
        so we only move them if all views inside it are unused """
        clone_folder_source = self.core.post_command(
            command="API.GetBuiltInContainerNavigatorItems",
            parameters={
                "navigatorItemIds": [{"navigatorItemId": undefined_node["navigatorItem"]["sourceNavigatorItemId"]}]
            },
        )
        number_of_source_navitems = len(clone_folder_source["navigatorItems"][0]["builtInContainerNavigatorItem"]["contentIds"])
        number_of_unused_navitems = len(undefined_node["navigatorItem"]["children"])
        if number_of_source_navitems == number_of_unused_navitems: # all views are unused
            with TeamworkReserve(self.conn_header, [undefined_node["navigatorItem"]["navigatorItemId"]]):
                self.core.post_command(
                    command="API.MoveNavigatorItem",
                    parameters={
                        "navigatorItemIdToMove": undefined_node["navigatorItem"]["navigatorItemId"],
                        "parentNavigatorItemId": parent_guid,
                    },
                )
            self.moved_views += number_of_unused_navitems
        return None


if __name__ == "__main__":
    try:
        moved_views = UnusedViewCleaner().run(MultiConn().primary)
        show_popup(message=f"Unused views has been successfully moved to Unused Views folder. \n"
                           f"Number of moved views: {moved_views}",
                   title="Success!",
                   message_type="info")
    except Exception as e:
        show_popup(message=f"An unexpected error has occurred during the execution of unused view cleaner. \n"
                           f"Error: {type(e).__name__} - {e}",
                   title="Error",
                   message_type="error")
        raise e
