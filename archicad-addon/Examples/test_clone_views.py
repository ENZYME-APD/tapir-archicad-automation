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


# 1. Find a source view in the Project Map
pm = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ProjectMap"}})
candidates = []
for t in ("StoryItem", "SectionItem", "ElevationItem", "WorksheetItem"):
    candidates = list(find_by_type(pm["navigatorTree"]["rootItem"]["children"], t))
    if candidates:
        break
assert candidates, "No clonable view found in Project Map"
source = candidates[0]
print(f"Source view: {source['name']} ({source['navigatorItemId']['guid']})")

# 2. Collect all View Map guids before
def collect_guids(branch, result=None):
    if result is None:
        result = set()
    for entry in branch:
        item = entry["navigatorItem"]
        nid = item.get("navigatorItemId", {})
        if nid.get("guid"):
            result.add(nid["guid"])
        children = item.get("children")
        if children:
            collect_guids(children, result)
    return result

vm_before = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
guids_before = collect_guids(vm_before["navigatorTree"]["rootItem"]["children"])
print(f"View Map item count before: {len(guids_before)}")

# 3. Clone it
result = run("CloneViewsToViewMap", {"viewsData": [{"navigatorItemId": source["navigatorItemId"]}]})
print(f"CloneViewsToViewMap result: {result}")
assert "navigatorItems" in result
clone_item = result["navigatorItems"][0]
assert "navigatorItemId" in clone_item, f"Clone failed: {clone_item}"
new_guid = clone_item["navigatorItemId"]["guid"]
print(f"New View Map item guid: {new_guid}")

# 4. Confirm the new guid is in the View Map
vm_after = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
guids_after = collect_guids(vm_after["navigatorTree"]["rootItem"]["children"])
print(f"View Map item count after: {len(guids_after)}")
assert new_guid in guids_after, f"New guid {new_guid} not found in View Map after cloning"

print("\nPASS: CloneViewsToViewMap created a new View Map item")
