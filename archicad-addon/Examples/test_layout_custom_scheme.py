"""
test_layout_custom_scheme.py

Validates:
  - GetLayoutCustomScheme  : returns Layout Info Panel field definitions (key + name)
  - GetLayoutSettings      : customData entries include customSchemeName
  - SetLayoutSettings      : customData settable by customSchemeKey (GUID)
                             customData settable by customSchemeName (field name)
"""
import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib


def run(cmd, p={}):
    return aclib.RunTapirCommand(cmd, p, debug=False)


# ---------------------------------------------------------------------------
# 1. GetLayoutCustomScheme
# ---------------------------------------------------------------------------
print("1. GetLayoutCustomScheme...")
scheme_resp = run("GetLayoutCustomScheme")
assert "customScheme" in scheme_resp, f"Missing customScheme: {scheme_resp}"
scheme = scheme_resp["customScheme"]
print(f"   {len(scheme)} field(s) defined in Layout Book:")
for f in scheme:
    print(f"     key={f['customSchemeKey']}  name={f['customSchemeName']!r}")
print("PASS: GetLayoutCustomScheme")

if not scheme:
    print("\n  [INFO] No custom scheme fields defined in this project.")
    print("  Define fields in Layout Book > Book Settings > Layout Scheme to test round-trips.")
    print("\nAll reachable tests passed.")
    sys.exit(0)

# ---------------------------------------------------------------------------
# 2. Find a regular layout to test on
# ---------------------------------------------------------------------------
print("\n2. Finding a regular layout...")
lb = aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})

def collect_layouts(branch, result=None):
    if result is None:
        result = []
    for entry in branch:
        item = entry["navigatorItem"]
        if item.get("type") == "LayoutItem":
            result.append(item)
        if item.get("children"):
            collect_layouts(item["children"], result)
    return result

layouts = collect_layouts(lb["navigatorTree"]["rootItem"]["children"])
assert layouts, "No LayoutItem found in Layout Book"
target = layouts[0]
# GetLayoutSettings uses "navigatorItemId"; SetLayoutSettings uses "layoutNavigatorItemId"
get_id = {"navigatorItemId":      target["navigatorItemId"]}
set_id = {"layoutNavigatorItemId": target["navigatorItemId"]}
print(f"   Using layout: {target.get('name', '?')}")

# Build a helper to pretty-print customData rows
def print_custom_data(custom_data):
    if not custom_data:
        print("     (aucune valeur renseignée)")
    for cd in custom_data:
        print(f"     [{cd.get('customSchemeName', cd['customSchemeKey'])}] = {cd['customSchemeValue']!r}")

# ---------------------------------------------------------------------------
# 3. Pre-fill all scheme fields with sample values, then read them back
# ---------------------------------------------------------------------------
print("\n3. Remplissage de tous les champs du schéma...")

# Remember original values before touching anything
orig_resp = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})
orig_settings = orig_resp["layoutSettings"][0]
assert "error" not in orig_settings, f"GetLayoutSettings error: {orig_settings}"
orig_data = {cd["customSchemeKey"]: cd["customSchemeValue"] for cd in orig_settings.get("customData", [])}

# Set sample values for every field in the scheme
sample_values = [
    {"customSchemeKey": f["customSchemeKey"], "customSchemeValue": f"Exemple_{f['customSchemeName']}"}
    for f in scheme
]
set_all = run("SetLayoutSettings", {"layoutsData": [{**set_id, "customData": sample_values}]})
assert set_all["executionResults"][0].get("success"), f"Pre-fill failed: {set_all}"

# Read back and display
filled_resp = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})
filled_settings = filled_resp["layoutSettings"][0]
assert "error" not in filled_settings, f"GetLayoutSettings error: {filled_settings}"
filled_data = filled_settings.get("customData", [])
print(f"   {len(filled_data)} champ(s) après remplissage :")
print_custom_data(filled_data)
for cd in filled_data:
    assert "customSchemeName" in cd, f"customSchemeName absent: {cd}"
print("PASS: GetLayoutSettings retourne customSchemeName avec les valeurs")

# ---------------------------------------------------------------------------
# 4. SetLayoutSettings by customSchemeKey (GUID)
# ---------------------------------------------------------------------------
print("\n4. SetLayoutSettings par customSchemeKey...")
field = scheme[0]
key   = field["customSchemeKey"]
name  = field["customSchemeName"]

test_value = "TAPIR_TEST_BY_KEY"
set_resp = run("SetLayoutSettings", {"layoutsData": [{
    **set_id,
    "customData": [{"customSchemeKey": key, "customSchemeValue": test_value}],
}]})
assert set_resp["executionResults"][0].get("success"), \
    f"SetLayoutSettings by key failed: {set_resp}"

verify = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})["layoutSettings"][0]
found = next((cd for cd in verify.get("customData", []) if cd["customSchemeKey"] == key), None)
assert found and found["customSchemeValue"] == test_value, \
    f"Valeur incorrecte après set-by-key: {found}"
print(f"   Résultat GetLayoutSettings après écriture par clé :")
print_custom_data(verify.get("customData", []))
print("PASS: SetLayoutSettings par customSchemeKey")

# ---------------------------------------------------------------------------
# 5. SetLayoutSettings by customSchemeName (field label)
# ---------------------------------------------------------------------------
print("\n5. SetLayoutSettings par customSchemeName...")
test_value2 = "TAPIR_TEST_BY_NAME"
set_resp2 = run("SetLayoutSettings", {"layoutsData": [{
    **set_id,
    "customData": [{"customSchemeName": name, "customSchemeValue": test_value2}],
}]})
assert set_resp2["executionResults"][0].get("success"), \
    f"SetLayoutSettings by name failed: {set_resp2}"

verify2 = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})["layoutSettings"][0]
found2 = next((cd for cd in verify2.get("customData", []) if cd["customSchemeKey"] == key), None)
assert found2 and found2["customSchemeValue"] == test_value2, \
    f"Valeur incorrecte après set-by-name: {found2}"
print(f"   Résultat GetLayoutSettings après écriture par nom :")
print_custom_data(verify2.get("customData", []))
print("PASS: SetLayoutSettings par customSchemeName")

# ---------------------------------------------------------------------------
# 6. Effacement d'un champ (valeur vide)
# ---------------------------------------------------------------------------
print(f"\n6. Effacement du champ {name!r} (valeur -> '')...")
run("SetLayoutSettings", {"layoutsData": [{
    **set_id,
    "customData": [{"customSchemeKey": key, "customSchemeValue": ""}],
}]})
after_clear = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})["layoutSettings"][0]
print(f"   Résultat après effacement :")
print_custom_data(after_clear.get("customData", []))

# ---------------------------------------------------------------------------
# 7. Restauration des valeurs d'origine
# ---------------------------------------------------------------------------
print("\n7. Restauration des valeurs d'origine...")
restore_data = [
    {"customSchemeKey": f["customSchemeKey"], "customSchemeValue": orig_data.get(f["customSchemeKey"], "")}
    for f in scheme
]
run("SetLayoutSettings", {"layoutsData": [{**set_id, "customData": restore_data}]})
restored = run("GetLayoutSettings", {"layoutDatabaseIds": [get_id]})["layoutSettings"][0]
print(f"   Valeurs restaurées :")
print_custom_data(restored.get("customData", []))

print("\nAll tests passed.")
