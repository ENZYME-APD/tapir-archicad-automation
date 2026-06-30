import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)


def find_by_type(branch, t):
    for entry in branch:
        item = entry["navigatorItem"]
        if item.get("type") == t:
            yield item
        children = item.get("children")
        if children:
            yield from find_by_type(children, t)


lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
layout = layouts[0]
print(f"Layout: {layout['name']}")

db_result = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": [{"navigatorItemId": layout["navigatorItemId"]}]})
layout_db_id = db_result["databases"][0]["databaseId"]

before = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id}]})
print(f"Before: {before}")

fake_guid = "12345678-1234-1234-1234-123456789ABC"
set_result = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": layout_db_id,
    "customData": [
        {"customSchemeKey": fake_guid, "customSchemeValue": "Hello Tapir"}
    ]
}]})
print(f"SetLayoutSettings: {set_result}")

after = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id}]})
print(f"After: {after}")

# Confirm other fields (customLayoutNumber etc.) were preserved (not wiped) when only customData was touched
custom_data = after["layoutSettings"][0].get("customData")
print(f"\ncustomData round-trip: {custom_data}")
