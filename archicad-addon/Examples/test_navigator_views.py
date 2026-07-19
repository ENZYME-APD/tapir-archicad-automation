"""
test_navigator_views.py

Validates:
  - CreateViewMapFolder
  - CreateViewsInViewMap        (independent views, isIndependent=True)
  - CloneProjectMapItemToViewMap (single linked view, NOT a live-sync folder)
  - SetViewRotation
  - GetViewSettings             (all fields + rotation round-trip)
  - GetView2DTransformations
  - SetViewSettings             (structureDisplay round-trip)
  - RenameNavigatorItem
  - DeleteNavigatorItems        (cleanup)
"""
import math
import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib

FOLDER_NAME = "TAPIR_TEST_VIEWS"
ANGLES = [(0.0, "Test North 0"), (45.0, "Test NE 45"), (90.0, "Test East 90")]


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)


def collect_items(branch, result=None):
    if result is None:
        result = []
    for entry in branch:
        item = entry["navigatorItem"]
        result.append(item)
        if item.get("children"):
            collect_items(item["children"], result)
    return result


# --- Project Map: get first StoryItem to use as a clone/view source ---
pm = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ProjectMap"}})
pm_items = collect_items(pm["navigatorTree"]["rootItem"]["children"])
story_sources = [it for it in pm_items if it.get("type") == "StoryItem"]
assert story_sources, "No StoryItem in Project Map — open a project with at least one floor plan"
src = story_sources[0]
print(f"Source StoryItem: {src.get('name', '?')}")

# 1. CreateViewMapFolder
print("\n1. CreateViewMapFolder...")
folder_resp = run("CreateViewMapFolder", {"folderName": FOLDER_NAME})
assert "navigatorItemId" in folder_resp, f"CreateViewMapFolder failed: {folder_resp}"
folder_id = folder_resp["navigatorItemId"]
print(f"PASS: CreateViewMapFolder -> {folder_id['guid']}")

# 2. CreateViewsInViewMap — 3 independent views with different names
print("\n2. CreateViewsInViewMap (3 views)...")
created = []
for deg, name in ANGLES:
    resp = run("CreateViewsInViewMap", {"viewsData": [{
        "navigatorItemId":       src["navigatorItemId"],
        "parentNavigatorItemId": folder_id,
        "name":                  name,
    }]})
    items_out = resp.get("navigatorItems", [])
    assert items_out and "navigatorItemId" in items_out[0], \
        f"CreateViewsInViewMap failed for '{name}': {items_out}"
    nav_id = items_out[0]["navigatorItemId"]
    created.append((deg, name, nav_id))
    print(f"  Created '{name}' ({nav_id['guid']})")
print(f"PASS: CreateViewsInViewMap -> {len(created)} independent views")

# 3. SetViewRotation for each view
print("\n3. SetViewRotation...")
rot_resp = run("SetViewRotation", {"navigatorItemIdsWithRotation": [
    {"navigatorItemId": nav_id, "rotation": math.radians(deg)}
    for deg, _, nav_id in created
]})
for i, (deg, name, _) in enumerate(created):
    assert rot_resp["executionResults"][i].get("success"), \
        f"SetViewRotation failed for '{name}': {rot_resp['executionResults'][i]}"
print("PASS: SetViewRotation applied")

# 4. Verify via GetViewSettings.rotation
print("\n4. GetViewSettings (rotation round-trip)...")
all_ids = [{"navigatorItemId": nav_id} for _, _, nav_id in created]
vs_resp = run("GetViewSettings", {"navigatorItemIds": all_ids})
assert "viewSettings" in vs_resp, "GetViewSettings returned no data"

for i, (deg, name, nav_id) in enumerate(created):
    vs = vs_resp["viewSettings"][i]
    assert "error" not in vs, f"GetViewSettings error for '{name}': {vs}"
    got_deg = math.degrees(vs["rotation"])
    assert abs(got_deg - deg) < 0.5, \
        f"Rotation mismatch for '{name}': expected {deg:.1f} deg, got {got_deg:.1f} deg"
    assert vs["saveZoom"] is True, f"saveZoom should be True for '{name}'"
    print(f"  {name:16s}  rotation={got_deg:.1f} deg  saveZoom={vs['saveZoom']}  OK")
print("PASS: GetViewSettings.rotation matches expected values")

# 4b. GetViewSettings — verify all expected fields
print("\n4b. GetViewSettings (all fields)...")
first_nav_id = created[0][2]
vs_full = run("GetViewSettings", {"navigatorItemIds": [{"navigatorItemId": first_nav_id}]})["viewSettings"][0]
assert "error" not in vs_full, f"GetViewSettings error: {vs_full}"
for field in [
    "modelViewOptions", "layerCombination", "dimensionStyle", "penSetName",
    "graphicOverrideCombination", "drawingScale", "saveZoom", "ignoreSavedZoom",
    "rotation", "structureDisplay", "usePhotoRendering", "d3styleName", "renderingSceneName",
]:
    assert field in vs_full, f"Missing field in GetViewSettings response: {field}"
print("PASS: GetViewSettings returns all expected fields")

# 4c. GetView2DTransformations
print("\n4c. GetView2DTransformations...")
db_transform = run(
    "GetView2DTransformations", {"navigatorItemIds": [{"navigatorItemId": first_nav_id}]}
)["transformations"][0]
assert "error" not in db_transform, f"GetView2DTransformations error: {db_transform}"
assert "rotation" in db_transform, "GetView2DTransformations: missing rotation field"
assert "zoom" in db_transform,     "GetView2DTransformations: missing zoom field"
print(f"PASS: GetView2DTransformations (DB zoom rotation = {math.degrees(db_transform['rotation']):.1f} deg)")

# 4d. SetViewSettings — structureDisplay round-trip
print("\n4d. SetViewSettings (structureDisplay round-trip)...")
original_structure = vs_full["structureDisplay"]
modes = ["EntireStructure", "CoreOnly", "WithoutFinishes", "StructureOnly"]
test_mode = modes[(modes.index(original_structure) + 1) % len(modes)]
sv_result = run("SetViewSettings", {"navigatorItemIdsWithViewSettings": [{
    "navigatorItemId": first_nav_id,
    "viewSettings": {"structureDisplay": test_mode},
}]})
assert sv_result["executionResults"][0].get("success"), \
    f"SetViewSettings (structureDisplay) failed: {sv_result}"
vs_after_sv = run("GetViewSettings", {"navigatorItemIds": [{"navigatorItemId": first_nav_id}]})["viewSettings"][0]
assert vs_after_sv["structureDisplay"] == test_mode, \
    f"structureDisplay round-trip failed: set {test_mode}, got {vs_after_sv['structureDisplay']}"
run("SetViewSettings", {"navigatorItemIdsWithViewSettings": [{
    "navigatorItemId": first_nav_id,
    "viewSettings": {"structureDisplay": original_structure},
}]})
print(f"PASS: SetViewSettings structureDisplay ({original_structure} -> {test_mode} -> restored)")

# 5. CloneProjectMapItemToViewMap — copy a single story view into test folder
print(f"\n5. CloneProjectMapItemToViewMap (source StoryItem -> {FOLDER_NAME})...")
clone_res = run("CloneProjectMapItemToViewMap", {
    "viewsData": [{"navigatorItemId": src["navigatorItemId"], "parentNavigatorItemId": folder_id}]
})
clone_items = clone_res.get("navigatorItems", [])
assert clone_items and "navigatorItemId" in clone_items[0], \
    f"CloneProjectMapItemToViewMap failed: {clone_items}"
clone_nav_id = clone_items[0]["navigatorItemId"]
print(f"   Linked item: {clone_nav_id['guid']}")
print("PASS: CloneProjectMapItemToViewMap")

# 6. RenameNavigatorItem — mark it as a linked view (not a live-sync clone folder)
print("\n6. RenameNavigatorItem (mark linked view)...")
rename_res = run("RenameNavigatorItem", {
    "navigatorItemId": clone_nav_id,
    "newName":         "TAPIR_LINKED_VIEW",
})
assert rename_res.get("success"), f"RenameNavigatorItem failed: {rename_res}"
print("PASS: RenameNavigatorItem -> 'TAPIR_LINKED_VIEW'")

# Visual check before cleanup
print(
    f"\n  [VISUAL CHECK] In the View Map, folder '{FOLDER_NAME}' should contain:\n"
    f"    - 3 independent views (Test North 0, Test NE 45, Test East 90) with rotations\n"
    f"    - 1 linked view named 'TAPIR_LINKED_VIEW' (NOT a folder)"
)

# 7. DeleteNavigatorItems — delete views + linked view, then test folder
print("\n7. DeleteNavigatorItems (cleanup)...")
all_view_ids = [{"navigatorItemId": nav_id} for _, _, nav_id in created] + \
               [{"navigatorItemId": clone_nav_id}]
del_views = run("DeleteNavigatorItems", {"navigatorItemIds": all_view_ids})
for i, r in enumerate(del_views["executionResults"]):
    assert r.get("success"), f"Delete view {i} failed: {r}"
print("   Views + linked view deleted.")

del_folder = run("DeleteNavigatorItems", {"navigatorItemIds": [{"navigatorItemId": folder_id}]})
assert del_folder["executionResults"][0].get("success"), \
    f"Delete test folder failed: {del_folder}"
print("   Test folder deleted.")

print("\nAll tests passed. View Map cleaned up.")
