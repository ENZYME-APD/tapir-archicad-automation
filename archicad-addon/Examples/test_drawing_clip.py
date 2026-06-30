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


# 1. Find layouts and their databases
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})
layouts = list(find_by_type(lb["navigatorTree"]["rootItem"]["children"], "LayoutItem"))
assert layouts, "No layout found"

layout_ids = [{"navigatorItemId": l["navigatorItemId"]} for l in layouts]
databases = run("GetDatabaseIdFromNavigatorItemId", {"navigatorItemIds": layout_ids})["databases"]

# 2. Find a Drawing element on a layout
drawings = run("GetElementsByType", {"elementType": "Drawing", "databases": databases})["elements"]
assert drawings, "No Drawing element found"
drawing = drawings[0]
print(f"Drawing elementId: {drawing['elementId']}")

# 3. Read current state
before = run("GetDetailsOfElements", {"elements": [drawing]})["detailsOfElements"][0]
bounds = before["details"]["bounds"]

# 4. Set a new triangle clip polygon (3 unique points, no closing vertex)
cx = (bounds["xMin"] + bounds["xMax"]) / 2
cy = (bounds["yMin"] + bounds["yMax"]) / 2
triangle = [
    {"x": bounds["xMin"], "y": bounds["yMin"]},
    {"x": bounds["xMax"], "y": bounds["yMin"]},
    {"x": cx,             "y": bounds["yMax"]},
]
set_result = run("SetDetailsOfElements", {"elementsWithDetails": [{
    "elementId": drawing["elementId"],
    "details": {"typeSpecificDetails": {"clipPolygon": triangle}}
}]})
assert set_result["executionResults"][0].get("success") is True, f"Set failed: {set_result}"
print("SET triangle (3 pts, no closing): OK")

# 5. Verify via GetDetailsOfElements
after = run("GetDetailsOfElements", {"elements": [drawing]})["detailsOfElements"][0]
assert after["details"].get("isCutWithFrame") is True, "isCutWithFrame should be True"
poly_after = after["details"].get("clipPolygon")
assert poly_after is not None, "clipPolygon missing after set"
# GetDetails returns coords with closing vertex included → triangle = 4 points (3 unique + 1 closing)
print(f"clipPolygon after set: {len(poly_after)} points (including closing vertex)")
assert len(poly_after) == 4, f"Expected 4 points (3 unique + closing), got {len(poly_after)}"

# 6. Round-trip: send the polygon back as-is (with closing vertex)
set_result2 = run("SetDetailsOfElements", {"elementsWithDetails": [{
    "elementId": drawing["elementId"],
    "details": {"typeSpecificDetails": {"clipPolygon": poly_after}}
}]})
assert set_result2["executionResults"][0].get("success") is True, f"Round-trip failed: {set_result2}"
print("Round-trip (4 pts with closing): OK")

print("\nPASS: Drawing clipPolygon SetDetailsOfElements OK")
