from typing import TypedDict, NotRequired, Container
from copy import deepcopy

VIEW_NAVITEM_TYPES = [
    "StoryItem",
    "SectionItem",
    "ElevationItem",
    "InteriorElevationItem",
    "WorksheetItem",
    "DetailItem",
    "DocumentFrom3DItem",
    "Perspective3DItem",
    "Axonometry3DItem",
    "CameraSetItem",
    "CameraItem",
    "ScheduleItem",
    "ProjectIndexItem",
    "TextListItem",
    "GraphicListItem",
]


class Guid(TypedDict):
    guid: str


class NavigatorItemId(TypedDict):
    navigatorItemId: Guid


class NavigatorItem(TypedDict):
    navigatorItemId: Guid
    prefix: str
    name: str
    type: str
    sourceNavigatorItemId: NotRequired[Guid]
    children: NotRequired[list['NavigatorItemWrapper']]


class NavigatorItemWrapper(TypedDict):
    navigatorItem: NavigatorItem


class NavigatorTreeId(TypedDict):
    type: str
    name: NotRequired[str]


class NavigatorTreeIdWrapper(TypedDict):
    navigatorTreeId: NavigatorTreeId

def get_unique_navigator_items_from_tree(
    current_branch: list[NavigatorItemWrapper], guid_navitem_map: dict[str, NavigatorItem], navitem_types: list[str]
):
    for navigator_item_wrapper in current_branch:
        navigator_item = navigator_item_wrapper["navigatorItem"]
        if navigator_item["type"] in navitem_types:
            guid_navitem_map.update({navigator_item["navigatorItemId"]["guid"]: navigator_item})
        children = navigator_item.get("children")
        if children:
            get_unique_navigator_items_from_tree(children, guid_navitem_map, navitem_types)


def partial_deepcopy(node: dict, keys_to_omit: Container[str]) -> dict:
    return {
        key: deepcopy(value) if isinstance(value, dict) else value
        for key, value in node.items()
        if key not in keys_to_omit
    }


def get_path_to_navitem(tree: NavigatorItemWrapper, navitem_id: NavigatorItem) -> list[NavigatorItemWrapper]:
    node_for_path = {"navigatorItem" :partial_deepcopy(tree["navigatorItem"], keys_to_omit="children")}
    if tree["navigatorItem"]["navigatorItemId"]["guid"] == navitem_id["navigatorItemId"]["guid"]:
        return [node_for_path] # Base case. We assume guids are unique
    for child in tree["navigatorItem"].get("children", []):
        if path:= get_path_to_navitem(child, navitem_id):
            return [node_for_path] + path
    return []


def get_child_with_guid(guid: str, children: list[NavigatorItemWrapper]) -> NavigatorItemWrapper | None:
    for child in children:
        if child["navigatorItem"]["navigatorItemId"]["guid"] == guid:
            return child
    return None


def merge_paths(paths_to_merge: list[list[NavigatorItemWrapper]]) -> NavigatorItemWrapper | dict:
    merged_tree = {}
    for path in paths_to_merge:
        tree_cursor = merged_tree
        for i, navitem in enumerate(path):
            guid = navitem["navigatorItem"]["navigatorItemId"]["guid"]
            if i == 0: # tree root
                if not tree_cursor:
                    tree_cursor.update(deepcopy(navitem))
                elif guid != tree_cursor["navigatorItem"]["navigatorItemId"]["guid"]:
                    raise AttributeError("All paths must share the same root item to be able to merge")
            else:
                current_children_of_cursor = tree_cursor["navigatorItem"].get("children", [])
                if child:= get_child_with_guid(guid, current_children_of_cursor):
                    tree_cursor = child
                else:
                    navitem_to_append = deepcopy(navitem)
                    tree_cursor["navigatorItem"]["children"] = current_children_of_cursor + [navitem_to_append]
                    tree_cursor = navitem_to_append
    return merged_tree