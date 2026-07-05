"""
test_navigator_views.py

Validates:
  - CreateViewMapFolder
  - CreateViewsInViewMap        (independent views, isIndependent=True)
  - CloneProjectMapItemToViewMap (single linked view, NOT a live-sync folder)
  - CreateViewMapCloneFolder    (true live-sync clone folder mirroring a Project Map folder)
  - SetViewRotation
  - GetViewSettings.rotation round-trip
  - RenameNavigatorItem
  - DeleteNavigatorItems (cleanup)
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


# --- Project Map: get first StoryItem and first folder/root for clone folder ---
pm = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ProjectMap"}})
pm_items = collect_items(pm["navigatorTree"]["rootItem"]["children"])
story_sources = [it for it in pm_items if it.get("type") == "StoryItem"]
assert story_sources, "No StoryItem in Project Map — open a project with at least one floor plan"
src = story_sources[0]
print(f"Source StoryItem: {src.get('name', '?')}")

# For CreateViewMapCloneFolder we need a Project Map folder (or root)
pm_root = pm["navigatorTree"]["rootItem"]
# rootItem has no "navigatorItem" wrapper — check direct navigatorItemId
pm_root_id = (
    pm_root.get("navigatorItemId") or
    pm_root.get("navigatorItem", {}).get("navigatorItemId") or
    src["navigatorItemId"]
)
print(f"Project Map source id for clone: {pm_root_id['guid']}")

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

# 7. CreateViewMapCloneFolder — live-syncing clone of Project Map root
print(f"\n7. CreateViewMapCloneFolder (clone Project Map root into {FOLDER_NAME})...")
clone_folder_res = run("CreateViewMapCloneFolder", {
    "sourceNavigatorItemId": pm_root_id,
    "parentNavigatorItemId": folder_id,
    "folderName":            "TAPIR_CLONE_FOLDER",
})
assert "navigatorItemId" in clone_folder_res, \
    f"CreateViewMapCloneFolder failed: {clone_folder_res}"
clone_folder_id = clone_folder_res["navigatorItemId"]
print(f"   Clone folder id: {clone_folder_id['guid']}")
print("PASS: CreateViewMapCloneFolder")

# Visual check before cleanup
print(
    f"\n  [VISUAL CHECK] In the View Map, folder '{FOLDER_NAME}' should contain:\n"
    f"    - 3 independent views (Test North 0, Test NE 45, Test East 90) with rotations\n"
    f"    - 1 linked view named 'TAPIR_LINKED_VIEW' (NOT a folder)\n"
    f"    - 1 clone folder named 'TAPIR_CLONE_FOLDER' mirroring Project Map root\n"
    f"  The clone folder content should auto-update when the Project Map changes."
)

# 8. DeleteNavigatorItems — delete views + linked view, then clone folder, then test folder
print("\n8. DeleteNavigatorItems (cleanup)...")
all_view_ids = [{"navigatorItemId": nav_id} for _, _, nav_id in created] + \
               [{"navigatorItemId": clone_nav_id}]
del_views = run("DeleteNavigatorItems", {"navigatorItemIds": all_view_ids})
for i, r in enumerate(del_views["executionResults"]):
    assert r.get("success"), f"Delete view {i} failed: {r}"
print("   Views + linked view deleted.")

del_clone_folder = run("DeleteNavigatorItems", {"navigatorItemIds": [{"navigatorItemId": clone_folder_id}]})
assert del_clone_folder["executionResults"][0].get("success"), \
    f"Delete clone folder failed: {del_clone_folder}"
print("   Clone folder deleted.")

del_folder = run("DeleteNavigatorItems", {"navigatorItemIds": [{"navigatorItemId": folder_id}]})
assert del_folder["executionResults"][0].get("success"), \
    f"Delete test folder failed: {del_folder}"
print("   Test folder deleted.")

print("\nAll tests passed. View Map cleaned up.")
