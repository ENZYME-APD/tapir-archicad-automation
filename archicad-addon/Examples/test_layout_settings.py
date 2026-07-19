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


# 1. Find or create a layout
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
if not layouts:
    print("No layout - creating one...")
    res = run("CreateLayouts", {"layoutsData": [{"masterLayoutName": "A4", "layoutName": "Test Layout Settings"}]})
    print(res)
    lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
    layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))

layout = layouts[0]
print(f"Layout: {layout['name']}")

# 2. Resolve its databaseId
db_result = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": [{"navigatorItemId": layout["navigatorItemId"]}]})
layout_db_id = db_result["databases"][0]["databaseId"]
print(f"Layout databaseId: {layout_db_id['guid']}")

# 3. GetLayoutSettings
get_result = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id}]})
print(f"GetLayoutSettings (before): {get_result}")

# 4. SetLayoutSettings - change customLayoutNumber and customLayoutNumbering
set_result = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": layout_db_id,
    "customLayoutNumber": "X99",
    "customLayoutNumbering": True,
}]})
print(f"SetLayoutSettings: {set_result}")

# 5. GetLayoutSettings again to confirm changes
get_result2 = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id}]})
print(f"GetLayoutSettings (after): {get_result2}")

after = get_result2["layoutSettings"][0]
assert after["customLayoutNumber"] == "X99", f"customLayoutNumber mismatch: {after['customLayoutNumber']}"
assert after["customLayoutNumbering"] is True, f"customLayoutNumbering mismatch: {after['customLayoutNumbering']}"
assert after["layoutName"].startswith("X99 "), f"layoutName should reflect the new custom ID: {after['layoutName']}"
print("\nPASS: customLayoutNumber / customLayoutNumbering round-trip OK")

# Reset for cleanliness
run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": layout_db_id,
    "customLayoutNumbering": False,
}]})
