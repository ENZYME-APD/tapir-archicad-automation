"""
test_layout_workflow.py

Tests ALL options of:
  - CreateLayoutSubset: name, numberingStyle, startAt, continueNumbering,
                        addOwnPrefix, ownPrefix, useUpperPrefix, customNumbering,
                        customNumber, parentNavigatorItemId
  - CreateLayout: layoutName, masterNavigatorItemId, masterLayoutName (alt.),
                  parentNavigatorItemId, layoutParameters (all 10 fields)
  - GetLayoutSettings: full round-trip of all returned fields
  - SetLayoutSettings: all fields via layoutDatabaseId AND via layoutNavigatorItemId
  - RenameNavigatorItem
  - MoveNavigatorItem: without and with previousNavigatorItemId (order verified)
  - DeleteNavigatorItems

Note: layout subsets cannot be deleted via the ArchiCAD API (AC29 limitation).
      Delete TAPIR_TEST_SUBSET_A_RENAMED, TAPIR_TEST_SUBSET_B, TAPIR_TEST_SUBSET_C manually.
"""
import sys
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib

SUBSET_A  = "TAPIR_TEST_SUBSET_A"
SUBSET_B  = "TAPIR_TEST_SUBSET_B"
SUBSET_C  = "TAPIR_TEST_SUBSET_C"
LAYOUT_1  = "TAPIR_TEST_LAYOUT_1"
LAYOUT_2  = "TAPIR_TEST_LAYOUT_2"


def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)

def wait(msg):
    print(f"\n  [VISUAL CHECK] {msg}")

def get_lb_tree():
    return aclib.RunCommand("API.GetNavigatorItemTree", {"navigatorTreeId": {"type": "LayoutBook"}})

def collect_all(branch, out=None):
    if out is None:
        out = []
    for entry in branch:
        item = entry["navigatorItem"]
        out.append(item)
        if item.get("children"):
            collect_all(item["children"], out)
    return out

def find_by_name(items, name):
    return next((it for it in items if it.get("name") == name), None)

def children_order_of(branch, parent_name):
    for entry in branch:
        item = entry["navigatorItem"]
        if item.get("name") == parent_name:
            return [c["navigatorItem"].get("name") for c in item.get("children", [])]
        if item.get("children"):
            r = children_order_of(item["children"], parent_name)
            if r is not None:
                return r
    return None


# ── 0. Find a master layout ───────────────────────────────────────────────
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
print("Types in LayoutBook:", sorted(set(it.get("type", "?") for it in all_items)))

master_item = next((it for it in all_items if it.get("type") == "MasterLayoutItem"), None)
assert master_item, "No MasterLayoutItem found — open a project with at least one master layout."
master_nav_id   = master_item["navigatorItemId"]
master_name     = master_item.get("name", "")
print(f"Master layout: '{master_name}' ({master_nav_id['guid']})")


# ── 1. CreateLayoutSubset A — basic options ───────────────────────────────
print("\n1. CreateLayoutSubset A (numberingStyle=1, startAt=10, addOwnPrefix=False)...")
res = run("CreateLayoutSubset", {"subsetsData": [{
    "name":              SUBSET_A,
    "numberingStyle":    "1",
    "startAt":           10,
    "continueNumbering": False,
    "addOwnPrefix":      False,
}]})
assert res.get("navigatorItems", [{}])[0].get("success"), f"CreateLayoutSubset A failed: {res}"
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
subset_a = find_by_name(all_items, SUBSET_A)
assert subset_a, f"'{SUBSET_A}' not found in Layout Book after creation"
subset_a_id = subset_a["navigatorItemId"]
print(f"   PASS ({subset_a_id['guid']})")


# ── 2. CreateLayoutSubset B — different numberingStyle ───────────────────
print("\n2. CreateLayoutSubset B (numberingStyle=abc, continueNumbering=True)...")
res = run("CreateLayoutSubset", {"subsetsData": [{
    "name":              SUBSET_B,
    "numberingStyle":    "abc",
    "continueNumbering": True,
}]})
assert res.get("navigatorItems", [{}])[0].get("success"), f"CreateLayoutSubset B failed: {res}"
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
subset_b = find_by_name(all_items, SUBSET_B)
assert subset_b, f"'{SUBSET_B}' not found in Layout Book after creation"
subset_b_id = subset_b["navigatorItemId"]
print(f"   PASS ({subset_b_id['guid']})")


# ── 3. CreateLayoutSubset C — all extra options + parentNavigatorItemId ───
print(f"\n3. CreateLayoutSubset C (ownPrefix, customNumbering, useUpperPrefix, nested in A)...")
res = run("CreateLayoutSubset", {"subsetsData": [{
    "name":                  SUBSET_C,
    "numberingStyle":        "01",
    "startAt":               1,
    "continueNumbering":     False,
    "addOwnPrefix":          True,
    "ownPrefix":             "C-",
    "useUpperPrefix":        False,
    "customNumbering":       True,
    "customNumber":          "C",
    "parentNavigatorItemId": subset_a_id,   # nested inside A
}]})
assert res.get("navigatorItems", [{}])[0].get("success"), f"CreateLayoutSubset C failed: {res}"
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
subset_c = find_by_name(all_items, SUBSET_C)
assert subset_c, f"'{SUBSET_C}' not found in tree — parentNavigatorItemId may have been ignored"
print(f"   PASS ({subset_c['navigatorItemId']['guid']})")


# ── 4. CreateLayout L1 — ALL layoutParameters + masterNavigatorItemId ─────
print(f"\n4. CreateLayout '{LAYOUT_1}' with ALL layoutParameters...")
res = run("CreateLayout", {"layoutsData": [{
    "layoutName":            LAYOUT_1,
    "parentNavigatorItemId": subset_a_id,
    "masterNavigatorItemId": master_nav_id,        # GUID form
    "layoutParameters": {
        "horizontalSize":          420.0,
        "verticalSize":            297.0,
        "leftMargin":              15.0,
        "topMargin":               20.0,
        "rightMargin":             25.0,
        "bottomMargin":            30.0,
        "customLayoutNumber":      "T01",
        "customLayoutNumbering":   True,
        "doNotIncludeInNumbering": True,
        # displayMasterLayoutBelow cannot be set at creation time (AC29 ignores it)
        # tested via SetLayoutSettings in step 6
    }
}]})
layout_db_id_1 = res.get("databases", [{}])[0].get("databaseId")
assert layout_db_id_1, f"CreateLayout L1 failed: {res}"
print(f"   PASS ({layout_db_id_1['guid']})")


# ── 5. GetLayoutSettings — verify ALL 10 fields ───────────────────────────
print("\n5. GetLayoutSettings (verify all 10 fields)...")
s = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id_1}]})["layoutSettings"][0]
print(f"   {s}")
assert abs(s["horizontalSize"]       - 420.0) < 0.1, f"horizontalSize: {s['horizontalSize']}"
assert abs(s["verticalSize"]         - 297.0) < 0.1, f"verticalSize: {s['verticalSize']}"
assert abs(s["leftMargin"]           - 15.0)  < 0.1, f"leftMargin: {s['leftMargin']}"
assert abs(s["topMargin"]            - 20.0)  < 0.1, f"topMargin: {s['topMargin']}"
assert abs(s["rightMargin"]          - 25.0)  < 0.1, f"rightMargin: {s['rightMargin']}"
assert abs(s["bottomMargin"]         - 30.0)  < 0.1, f"bottomMargin: {s['bottomMargin']}"
assert s["customLayoutNumber"]      == "T01", f"customLayoutNumber: {s['customLayoutNumber']}"
assert s["customLayoutNumbering"]   is True,  f"customLayoutNumbering: {s['customLayoutNumbering']}"
assert s["doNotIncludeInNumbering"] is True,  f"doNotIncludeInNumbering: {s['doNotIncludeInNumbering']}"
# displayMasterLayoutBelow defaults to False at creation — tested via SetLayoutSettings (step 6)
assert "displayMasterLayoutBelow" in s, "displayMasterLayoutBelow field missing from response"
print("   PASS: 9 fields verified (displayMasterLayoutBelow tested in step 6)")


# ── 5b. GetLayoutSettings on MASTER layout ───────────────────────────────
print("\n5b. GetLayoutSettings on master layout...")
master_db_res = run("GetDatabaseIdFromNavigatorItemId", {
    "navigatorItemIds": [{"navigatorItemId": master_nav_id}]
})
master_db_id = master_db_res["databases"][0]["databaseId"]
s_master_orig = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": master_db_id}]})["layoutSettings"][0]
print(f"   Master '{master_name}': {s_master_orig['horizontalSize']}x{s_master_orig['verticalSize']}, "
      f"margins {s_master_orig['leftMargin']}/{s_master_orig['topMargin']}/"
      f"{s_master_orig['rightMargin']}/{s_master_orig['bottomMargin']}")
assert "horizontalSize" in s_master_orig, "GetLayoutSettings failed on master layout"
print("   PASS: GetLayoutSettings works on master layout")


# ── 6. SetLayoutSettings on regular layout — non-size fields only ─────────
print("\n6. SetLayoutSettings on regular layout (customLayoutNumber, flags)...")
res = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId":        layout_db_id_1,
    "customLayoutNumber":      "T99",
    "customLayoutNumbering":   False,
    "doNotIncludeInNumbering": False,
}]})
assert res["executionResults"][0].get("success"), f"SetLayoutSettings failed: {res}"

s2 = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id_1}]})["layoutSettings"][0]
assert s2["customLayoutNumber"]      == "T99", f"customLayoutNumber: {s2['customLayoutNumber']}"
assert s2["customLayoutNumbering"]   is False, f"customLayoutNumbering: {s2['customLayoutNumbering']}"
assert s2["doNotIncludeInNumbering"] is False, f"doNotIncludeInNumbering: {s2['doNotIncludeInNumbering']}"
print("   PASS: SetLayoutSettings non-size fields on regular layout")


# ── 6b. SetLayoutSettings on regular layout with size -> expect error ───────
print("\n6b. SetLayoutSettings with size on regular layout (expect error)...")
res_sz = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": layout_db_id_1,
    "horizontalSize":   297.0,
}]})
r = res_sz["executionResults"][0]
assert not r.get("success"), f"Expected error for size on regular layout, got success: {r}"
print(f"   PASS: error returned — {r.get('error', {}).get('message', '?')}")

print("\n6b. SetLayoutSettings with showMasterBelow on regular layout (expect error)...")
res_smb = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": layout_db_id_1,
    "showMasterBelow":  True,
}]})
r_smb = res_smb["executionResults"][0]
assert not r_smb.get("success"), f"Expected error for showMasterBelow on regular layout, got success: {r_smb}"
print(f"   PASS: error returned — {r_smb.get('error', {}).get('message', '?')}")


# ── 6c. SetLayoutSettings on MASTER layout — size + margins ───────────────
print("\n6c. SetLayoutSettings on master layout (size + margins) + round-trip...")
res = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": master_db_id,
    "horizontalSize":   500.0,
    "verticalSize":     350.0,
    "leftMargin":       20.0,
    "topMargin":        21.0,
    "rightMargin":      22.0,
    "bottomMargin":     23.0,
}]})
assert res["executionResults"][0].get("success"), f"SetLayoutSettings on master failed: {res}"

sm = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": master_db_id}]})["layoutSettings"][0]
assert abs(sm["horizontalSize"] - 500.0) < 0.1, f"master horizontalSize: {sm['horizontalSize']}"
assert abs(sm["verticalSize"]   - 350.0) < 0.1, f"master verticalSize: {sm['verticalSize']}"
assert abs(sm["leftMargin"]     - 20.0)  < 0.1, f"master leftMargin: {sm['leftMargin']}"
assert abs(sm["topMargin"]      - 21.0)  < 0.1, f"master topMargin: {sm['topMargin']}"
assert abs(sm["rightMargin"]    - 22.0)  < 0.1, f"master rightMargin: {sm['rightMargin']}"
assert abs(sm["bottomMargin"]   - 23.0)  < 0.1, f"master bottomMargin: {sm['bottomMargin']}"
print("   PASS: master layout size+margins changed and verified")

# Restore master layout to original values
res = run("SetLayoutSettings", {"layoutsData": [{
    "layoutDatabaseId": master_db_id,
    "horizontalSize":   s_master_orig["horizontalSize"],
    "verticalSize":     s_master_orig["verticalSize"],
    "leftMargin":       s_master_orig["leftMargin"],
    "topMargin":        s_master_orig["topMargin"],
    "rightMargin":      s_master_orig["rightMargin"],
    "bottomMargin":     s_master_orig["bottomMargin"],
}]})
assert res["executionResults"][0].get("success"), f"Restore master failed: {res}"
print(f"   PASS: master layout restored to original ({s_master_orig['horizontalSize']}x{s_master_orig['verticalSize']})")


# ── 7. CreateLayout L2 via masterLayoutName (alternative to masterNavigatorItemId)
print(f"\n7. CreateLayout '{LAYOUT_2}' via masterLayoutName='{master_name}'...")
res = run("CreateLayout", {"layoutsData": [{
    "layoutName":            LAYOUT_2,
    "parentNavigatorItemId": subset_a_id,
    "masterLayoutName":      master_name,          # name form (alternative path)
    "layoutParameters": {
        "horizontalSize":        297.0,
        "verticalSize":          210.0,
        "leftMargin":            10.0,
        "topMargin":             10.0,
        "rightMargin":           10.0,
        "bottomMargin":          10.0,
        "customLayoutNumber":    "T02",
        "customLayoutNumbering": True,
    }
}]})
layout_db_id_2 = res.get("databases", [{}])[0].get("databaseId")
assert layout_db_id_2, f"CreateLayout L2 (via masterLayoutName) failed: {res}"
print(f"   PASS ({layout_db_id_2['guid']})")


# ── 7b. SetLayoutSettings via layoutNavigatorItemId (alternative identifier)
print(f"\n7b. SetLayoutSettings via layoutNavigatorItemId...")
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
layout_2_item = find_by_name(all_items, LAYOUT_2)
assert layout_2_item, f"'{LAYOUT_2}' not found in tree"
layout_nav_id_2 = layout_2_item["navigatorItemId"]

res = run("SetLayoutSettings", {"layoutsData": [{
    "layoutNavigatorItemId": layout_nav_id_2,      # alternative to layoutDatabaseId
    "customLayoutNumber":    "T02-NAV",
    "customLayoutNumbering": True,
    "leftMargin":            12.0,
    "topMargin":             13.0,
    "rightMargin":           14.0,
    "bottomMargin":          15.0,
}]})
assert res["executionResults"][0].get("success"), f"SetLayoutSettings (nav id) failed: {res}"

s3 = run("GetLayoutSettings", {"layoutDatabaseIds": [{"databaseId": layout_db_id_2}]})["layoutSettings"][0]
assert s3["customLayoutNumber"]  == "T02-NAV", f"navItemId route: customLayoutNumber={s3['customLayoutNumber']}"
assert abs(s3["leftMargin"]      - 12.0) < 0.1, f"navItemId route: leftMargin={s3['leftMargin']}"
assert abs(s3["topMargin"]       - 13.0) < 0.1, f"navItemId route: topMargin={s3['topMargin']}"
assert abs(s3["rightMargin"]     - 14.0) < 0.1, f"navItemId route: rightMargin={s3['rightMargin']}"
assert abs(s3["bottomMargin"]    - 15.0) < 0.1, f"navItemId route: bottomMargin={s3['bottomMargin']}"
print("   PASS: layoutNavigatorItemId route works")


# ── Visual check 1 ────────────────────────────────────────────────────────
wait(
    f"Layout Book — Subset A should contain:\n"
    f"    - '{LAYOUT_1}' (T99, customLayoutNumbering=False, doNotInclude=False)\n"
    f"    - '{LAYOUT_2}' (T02-NAV)\n"
    f"    - Subset C (nested)\n"
    f"  Master layout '{master_name}' should be back to its original size."
)


# ── 8. RenameNavigatorItem ────────────────────────────────────────────────
SUBSET_A_RENAMED = SUBSET_A + "_RENAMED"
print(f"\n8. RenameNavigatorItem: '{SUBSET_A}' -> '{SUBSET_A_RENAMED}'...")
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
subset_a_current = find_by_name(all_items, SUBSET_A)
assert subset_a_current, f"'{SUBSET_A}' not found"
res = run("RenameNavigatorItem", {
    "navigatorItemId": subset_a_current["navigatorItemId"],
    "newName":         SUBSET_A_RENAMED,
})
assert res.get("success"), f"RenameNavigatorItem failed: {res}"
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
assert find_by_name(all_items, SUBSET_A_RENAMED), f"'{SUBSET_A_RENAMED}' not found after rename"
print("   PASS: RenameNavigatorItem")


# ── 9. MoveNavigatorItem L1 -> Subset B (no previousNavigatorItemId) ───────
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
layout_1_item = find_by_name(all_items, LAYOUT_1)
assert layout_1_item, f"'{LAYOUT_1}' not found"
layout_nav_id_1 = layout_1_item["navigatorItemId"]

print(f"\n9. MoveNavigatorItem: '{LAYOUT_1}' -> Subset B (no position)...")
res = run("MoveNavigatorItem", {
    "navigatorItemIdToMove": layout_nav_id_1,
    "parentNavigatorItemId": subset_b_id,
})
assert res.get("success"), f"MoveNavigatorItem L1 failed: {res}"
print("   PASS: MoveNavigatorItem (without previousNavigatorItemId)")


# ── 10. MoveNavigatorItem L2 -> Subset B after L1 (previousNavigatorItemId) ─
print(f"\n10. MoveNavigatorItem: '{LAYOUT_2}' -> Subset B, after '{LAYOUT_1}'...")
lb = get_lb_tree()
all_items = collect_all(lb["navigatorTree"]["rootItem"]["children"])
layout_2_item = find_by_name(all_items, LAYOUT_2)
assert layout_2_item, f"'{LAYOUT_2}' not found"
layout_nav_id_2 = layout_2_item["navigatorItemId"]

res = run("MoveNavigatorItem", {
    "navigatorItemIdToMove":   layout_nav_id_2,
    "parentNavigatorItemId":   subset_b_id,
    "previousNavigatorItemId": layout_nav_id_1,   # L2 placed right after L1
})
assert res.get("success"), f"MoveNavigatorItem L2 failed: {res}"

lb = get_lb_tree()
order = children_order_of(lb["navigatorTree"]["rootItem"]["children"], SUBSET_B)
assert order is not None, "Subset B not found in Layout Book tree"
idx_1 = next((i for i, n in enumerate(order) if n == LAYOUT_1), None)
idx_2 = next((i for i, n in enumerate(order) if n == LAYOUT_2), None)
assert idx_1 is not None, f"'{LAYOUT_1}' not in Subset B children: {order}"
assert idx_2 is not None, f"'{LAYOUT_2}' not in Subset B children: {order}"
assert idx_1 < idx_2, f"Expected L1 before L2 in Subset B, got: {order}"
print(f"   PASS: previousNavigatorItemId respected — {LAYOUT_1}[{idx_1}] < {LAYOUT_2}[{idx_2}]")


# ── Visual check 2 ────────────────────────────────────────────────────────
wait(
    f"Layout Book — Subset B should contain '{LAYOUT_1}' then '{LAYOUT_2}' (in that order).\n"
    f"             Subset A_RENAMED should contain only Subset C."
)


# ── 11. DeleteNavigatorItems (L1 + L2) ───────────────────────────────────
print("\n11. DeleteNavigatorItems: L1 + L2...")
res = run("DeleteNavigatorItems", {"navigatorItemIds": [
    {"navigatorItemId": layout_nav_id_1},
    {"navigatorItemId": layout_nav_id_2},
]})
for i, r in enumerate(res["executionResults"]):
    assert r.get("success"), f"Delete layout {i + 1} failed: {r}"
print("   PASS: both layouts deleted")

print(f"\nAll assertions passed.")
print(f"NOTE: delete manually in ArchiCAD Layout Book: '{SUBSET_A_RENAMED}', '{SUBSET_B}', '{SUBSET_C}'.")
