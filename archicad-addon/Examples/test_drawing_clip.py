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


# 1. Find a layout and its database
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
assert layouts, "No layout found"

layout = layouts[0]
layout_ids = [{"navigatorItemId": layout["navigatorItemId"]}]
db_result = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": layout_ids})
layout_db_id = db_result["databases"][0]["databaseId"]

# 2. Find a StoryItem (floor plan view) to place as Drawing
vt = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ProjectMap"}})
views = list(find_by_type(vt["navigatorTree"]["rootItem"]["children"], "StoryItem"))
assert views, "No StoryItem found in ProjectMap"
nav_item_id = views[0]["navigatorItemId"]
print(f"StoryItem navigatorItemId: {nav_item_id}")

# 3. Create a Drawing on the layout WITH a triangle clip polygon
cx, cy = 0.1, 0.17
triangle = [
    {"x": 0.02, "y": 0.06},
    {"x": 0.18, "y": 0.06},
    {"x": cx,   "y": 0.28},
]
create_result = run("CreateDrawings", {"drawingsData": [{
    "navigatorItemId": nav_item_id,
    "layoutDatabaseId": layout_db_id,
    "name": "TestClipDrawing",
    "position": {"x": cx, "y": cy},
    "clipPolygon": triangle,
}]})
print(f"CreateDrawings result: {create_result}")
created = create_result["elements"][0]
assert "elementId" in created, f"CreateDrawings failed: {created}"
drawing_ref = created  # {"elementId": {"guid": "..."}}
print(f"Created Drawing: {drawing_ref['elementId']}")

# 4. Verify via GetDetailsOfElements
after = run("GetDetailsOfElements", {"elements": [drawing_ref]})["detailsOfElements"][0]
print(f"isCutWithFrame: {after['details'].get('isCutWithFrame')}")
poly = after["details"].get("clipPolygon")
assert poly is not None, "clipPolygon missing after create"
print(f"clipPolygon points: {len(poly)}")
for i, pt in enumerate(poly):
    print(f"  pt[{i}]: ({pt['x']:.6f}, {pt['y']:.6f})")

assert after["details"].get("isCutWithFrame") is True, "isCutWithFrame should be True"
assert len(poly) == 4, f"Expected 4 points (triangle + closing), got {len(poly)}"

# 5. Clean up
del_result = run("DeleteElements", {"elements": [drawing_ref]})
print(f"Delete: {del_result}")

print("\nPASS: CreateDrawings with clipPolygon OK")
