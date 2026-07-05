"""
test_view_settings.py

Validates extended GetViewSettings / SetViewSettings / SetViewRotation fields:
  - GetViewSettings returns rotation, structureDisplay, usePhotoRendering,
    d3styleName, renderingSceneName, (optionally) renovationFilterGuid
  - SetViewRotation + GetViewSettings.rotation round-trip
  - GetView2DTransformations accepts navigatorItemIds
  - SetViewSettings structureDisplay round-trip
"""
import math
import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)


def collect_items(branch, result=None):
    if result is None:
        result = []
    for entry in branch:
        item = entry["navigatorItem"]
        result.append(item)
        children = item.get("children")
        if children:
            collect_items(children, result)
    return result


# 1. Find any StoryItem in the View Map
vm = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "ViewMap"}})
all_items = collect_items(vm["navigatorTree"]["rootItem"]["children"])
story_items = [it for it in all_items if it.get("type") == "StoryItem"]
assert story_items, "No StoryItem in View Map — open a project with at least one floor plan view"
target = story_items[0]
nav_id = {"navigatorItemId": target["navigatorItemId"]}
print(f"Testing on view: {target.get('name', '?')} ({target['navigatorItemId']['guid']})")

# 2. GetViewSettings — verify expected fields are present
settings_resp = run("GetViewSettings", {"navigatorItemIds": [nav_id]})
assert "viewSettings" in settings_resp, "GetViewSettings returned no viewSettings"
vs = settings_resp["viewSettings"][0]
assert "error" not in vs, f"GetViewSettings error: {vs}"

required_fields = [
    "modelViewOptions", "layerCombination", "dimensionStyle", "penSetName",
    "graphicOverrideCombination", "drawingScale", "saveZoom", "ignoreSavedZoom",
    "rotation", "structureDisplay", "usePhotoRendering",
    "d3styleName", "renderingSceneName",
]
for field in required_fields:
    assert field in vs, f"Missing field in GetViewSettings response: {field}"

print(f"  rotation          = {vs['rotation']:.6f} rad  ({math.degrees(vs['rotation']):.2f} deg)")
print(f"  structureDisplay  = {vs['structureDisplay']}")
print(f"  usePhotoRendering = {vs['usePhotoRendering']}")
if "renovationFilterGuid" in vs:
    print(f"  renovationFilterGuid = {vs['renovationFilterGuid']['guid']}")
print(f"  d3styleName       = '{vs.get('d3styleName', '')}'")
print(f"  renderingSceneName = '{vs.get('renderingSceneName', '')}'")
print("PASS: GetViewSettings returns all expected fields")

# 3. SetViewRotation + GetViewSettings.rotation round-trip
# GetViewSettings.rotation reads from navigatorView.tr (per-item, set by SetViewRotation).
# GetView2DTransformations reads from the shared database zoom (not affected by SetViewRotation).
original_rotation = vs["rotation"]
test_rotation = original_rotation + math.pi / 6  # +30 degrees

result = run("SetViewRotation", {"navigatorItemIdsWithRotation": [{
    "navigatorItemId": target["navigatorItemId"],
    "rotation": test_rotation,
}]})
assert result["executionResults"][0].get("success"), f"SetViewRotation failed: {result}"

vs_after = run("GetViewSettings", {"navigatorItemIds": [nav_id]})["viewSettings"][0]
got_rotation = vs_after["rotation"]
assert abs(got_rotation - test_rotation) < 1e-5, (
    f"SetViewRotation round-trip failed: "
    f"set {math.degrees(test_rotation):.2f} deg, got {math.degrees(got_rotation):.2f} deg"
)
print(
    f"PASS: SetViewRotation round-trip  "
    f"({math.degrees(original_rotation):.1f} -> {math.degrees(test_rotation):.1f} -> "
    f"{math.degrees(got_rotation):.1f} deg)"
)

# 4. GetView2DTransformations accepts navigatorItemIds (DB zoom, independent of SetViewRotation)
db_transform = run("GetView2DTransformations", {"navigatorItemIds": [nav_id]})["transformations"][0]
assert "error" not in db_transform, f"GetView2DTransformations error: {db_transform}"
assert "rotation" in db_transform, "GetView2DTransformations: missing rotation field"
assert "zoom" in db_transform,     "GetView2DTransformations: missing zoom field"
print(
    f"PASS: GetView2DTransformations via navigatorItemId  "
    f"(DB zoom rotation = {math.degrees(db_transform['rotation']):.1f} deg)"
)

# Restore original rotation before continuing
run("SetViewRotation", {"navigatorItemIdsWithRotation": [{
    "navigatorItemId": target["navigatorItemId"],
    "rotation": original_rotation,
}]})

# 5. SetViewSettings — structureDisplay round-trip
original_structure = vs["structureDisplay"]
modes = ["EntireStructure", "CoreOnly", "WithoutFinishes", "StructureOnly"]
test_mode = modes[(modes.index(original_structure) + 1) % len(modes)]

result = run("SetViewSettings", {"navigatorItemIdsWithViewSettings": [{
    "navigatorItemId": target["navigatorItemId"],
    "viewSettings": {"structureDisplay": test_mode},
}]})
assert result["executionResults"][0].get("success"), \
    f"SetViewSettings (structureDisplay) failed: {result}"

vs_after2 = run("GetViewSettings", {"navigatorItemIds": [nav_id]})["viewSettings"][0]
assert vs_after2["structureDisplay"] == test_mode, (
    f"structureDisplay round-trip failed: set {test_mode}, got {vs_after2['structureDisplay']}"
)
print(f"PASS: structureDisplay round-trip  ({original_structure} -> {test_mode})")

# Restore original structureDisplay
run("SetViewSettings", {"navigatorItemIdsWithViewSettings": [{
    "navigatorItemId": target["navigatorItemId"],
    "viewSettings": {"structureDisplay": original_structure},
}]})
vs_restored = run("GetViewSettings", {"navigatorItemIds": [nav_id]})["viewSettings"][0]
assert vs_restored["structureDisplay"] == original_structure, "Restore structureDisplay failed"
print("PASS: original values restored")

print("\nAll tests passed.")
