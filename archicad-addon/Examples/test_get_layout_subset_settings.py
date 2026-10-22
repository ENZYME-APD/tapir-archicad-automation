import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)


# 1. Create a subset with specific ID/numbering settings
create_result = run("CreateLayoutSubset", {"subsetsData": [{
    "name": "Test Subset Settings",
    "ownPrefix": "TS-",
    "customNumber": "7",
    "customNumbering": True,
    "numberingStyle": "01",
    "startAt": 3,
    "continueNumbering": False,
    "useUpperPrefix": False,
    "includeToIDSequence": True,
}]})
print(f"CreateLayoutSubset: {create_result}")
subset_item_id = create_result["navigatorItems"][0]["navigatorItemId"]

# 2. GetLayoutSubsetSettings - read the settings back
get_result = run("GetLayoutSubsetSettings", {"navigatorItemIds": [subset_item_id]})
print(f"GetLayoutSubsetSettings: {get_result}")

settings = get_result["subsetSettings"][0]
assert settings["name"] == "Test Subset Settings", f"name mismatch: {settings['name']}"
assert settings["ownPrefix"] == "TS-", f"ownPrefix mismatch: {settings['ownPrefix']}"
assert settings["addOwnPrefix"] is True, f"addOwnPrefix mismatch: {settings['addOwnPrefix']}"
assert settings["customNumber"] == "7", f"customNumber mismatch: {settings['customNumber']}"
assert settings["customNumbering"] is True, f"customNumbering mismatch: {settings['customNumbering']}"
assert settings["numberingStyle"] == "01", f"numberingStyle mismatch: {settings['numberingStyle']}"
assert settings["startAt"] == 3, f"startAt mismatch: {settings['startAt']}"
assert settings["includeToIDSequence"] is True, f"includeToIDSequence mismatch: {settings['includeToIDSequence']}"

print("\nPASS: GetLayoutSubsetSettings reflects the settings given at creation")
