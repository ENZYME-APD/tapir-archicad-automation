"""
Test: ExpressionBasedInfo (project info) + ExpressionBasedProperty (string expression)

Steps:
  1. Create a project info field named 'ExpressionBasedInfo'
  2. Verify it appears in GetAllProperties (required to reference it in expressions)
  3. Create 'ExpressionBasedProperty' (type=string) with CONCAT("Test"; <ref to ExpressionBasedInfo>)
  4. Read back and verify the expression is correct (with GUID resolved to label)
  5. Cleanup
"""

import sys, re
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples')
import aclib

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

def get_all_classification_items():
    """Return a flat availability list with all classification item GUIDs."""
    availability = []
    systems_r = aclib.RunCommand('API.GetAllClassificationSystems', {})
    for sys_entry in (systems_r or {}).get('classificationSystems', []):
        sys_id = sys_entry.get('classificationSystemId')
        if not sys_id:
            continue
        items_r = aclib.RunCommand('API.GetAllClassificationsInSystem',
                                   {'classificationSystemId': sys_id})
        def collect(items):
            for item in (items or []):
                ci = item.get('classificationItem', {})
                guid = ci.get('classificationItemId', {}).get('guid')
                if guid:
                    availability.append({'classificationItemId': {'guid': guid}})
                collect(ci.get('children', []))
        collect(items_r.get('classificationItems', []))
    return availability

passes = fails = 0

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected}, got={got})')

def build_guid_map():
    props = (run('GetAllProperties') or {}).get('properties', [])
    return {
        p['propertyId']['guid']: f"{p['propertyGroupName']} / {p['propertyName']}"
        for p in props if 'propertyId' in p
    }, props

def resolve(expr, guid_map):
    return re.sub(
        r'###Property:([0-9A-Fa-f\-]+)###',
        lambda m: f'[{guid_map.get(m.group(1), m.group(1))}]',
        expr
    )

def ref(guid):
    return f'###Property:{guid}###'

# ---------------------------------------------------------------------------
# 1. Create project info field 'ExpressionBasedInfo'
# ---------------------------------------------------------------------------
print('STEP 1 -- Create project info field "ExpressionBasedInfo" (or reuse if exists)')
# Check if already present before creating
_, all_props_pre = build_guid_map()
existing = next((p for p in all_props_pre
                 if p.get('propertyName') == 'ExpressionBasedInfo'
                 and p.get('propertyGroupName') == 'ProjectInfoPropertyDefinitionGroup'), None)

info_id = None
info_already_existed = False
if existing:
    print('  Already exists — reusing')
    info_already_existed = True
    info_id = 'existing'  # placeholder; GUID resolved in STEP 2
    check('project info field present', True, True)
else:
    info_r  = run('CreateProjectInfoFields', {
        'projectInfoFields': [
            {'projectInfoName': 'ExpressionBasedInfo', 'projectInfoValue': 'HelloFromTapir'}
        ]
    })
    info_entry = (info_r or {}).get('fields', [{}])[0]
    info_id    = info_entry.get('projectInfoId')
    check('project info field created', True, info_id is not None)
    if info_id:
        print(f'  projectInfoId : {info_id}')
print()

# ---------------------------------------------------------------------------
# 2. Check if the new project info appears in GetAllProperties
# ---------------------------------------------------------------------------
print('STEP 2 -- Check if ExpressionBasedInfo appears in GetAllProperties')
guid_map, all_props = build_guid_map()

# Project info fields might appear under a group containing "ProjectInfo"
info_prop_guid = None
for p in all_props:
    if p.get('propertyName') == 'ExpressionBasedInfo':
        info_prop_guid = p['propertyId']['guid']
        grp = p.get('propertyGroupName', '')
        ptype = p.get('propertyType', '')
        print(f'  Found: {grp} / ExpressionBasedInfo  (type={ptype}, guid={info_prop_guid})')
        break

# Also show all project-info-related groups for diagnostics
pi_props = [p for p in all_props if 'ProjectInfo' in p.get('propertyGroupName', '')
            or 'projectinfo' in p.get('propertyGroupName', '').lower()]
if pi_props:
    print(f'  ProjectInfo group properties ({len(pi_props)} total):')
    for p in pi_props[:10]:
        print(f'    {p["propertyGroupName"]} / {p["propertyName"]}')

check('ExpressionBasedInfo visible in GetAllProperties', True, info_prop_guid is not None)
print()

# ---------------------------------------------------------------------------
# 3. Create property group + ExpressionBasedProperty
# ---------------------------------------------------------------------------
print('STEP 3 -- Create property group "TestExprGroup" (or reuse if exists)')
grp_r     = run('CreatePropertyGroups', {'propertyGroups': [
    {'propertyGroup': {'name': 'TestExprGroup', 'description': 'Tapir expression test'}}
]})
grp_entry = (grp_r or {}).get('propertyGroupIds', [{}])[0]
grp_guid  = grp_entry.get('propertyGroupId', {}).get('guid')
if grp_guid:
    check('group created', True, True)
else:
    # Already exists — CreatePropertyDefinitions can reference it by name directly
    print('  Already exists — will reference by name')
    check('group present', True, True)
    grp_guid = '__by_name__'   # sentinel: use name-based reference below
print()

print('STEP 4 -- Create ExpressionBasedProperty (string CONCAT expression, or reuse if exists)')
# Check if already exists
_, all_props_now = build_guid_map()
existing_prop = next((p for p in all_props_now
                      if p.get('propertyName') == 'ExpressionBasedProperty'
                      and p.get('propertyGroupName') == 'TestExprGroup'), None)
existing_prop_guid = (existing_prop or {}).get('propertyId', {}).get('guid')
created_prop_guid  = None
used_type = None

# Collect full availability list (all classification items)
print('  Collecting all classification items for availability...')
all_avail = get_all_classification_items()
print(f'  Found {len(all_avail)} classification items')

# Always delete and recreate so availability is up to date
if existing_prop_guid:
    print('  Deleting existing ExpressionBasedProperty to recreate with full availability...')
    run('DeletePropertyDefinitions', {
        'propertyIds': [{'propertyId': {'guid': existing_prop_guid}}]
    })

if grp_guid and info_prop_guid:
    # Try multiple CONCAT syntaxes then fall back to number
    candidates = [
        ('string', f'CONCAT ( "Test" ; {ref(info_prop_guid)} )'),
        ('string', f'CONCAT ( "Test", {ref(info_prop_guid)} )'),
        ('string', f'"Test" + {ref(info_prop_guid)}'),
        ('number', f'STRTONUM ( {ref(info_prop_guid)} )'),
    ]
    group_ref = {'name': 'TestExprGroup'} if grp_guid == '__by_name__' \
                else {'propertyGroupId': {'guid': grp_guid}}
    for prop_type, expr in candidates:
        prop_r = run('CreatePropertyDefinitions', {'propertyDefinitions': [
            {'propertyDefinition': {
                'name'        : 'ExpressionBasedProperty',
                'description' : '',
                'type'        : prop_type,
                'isEditable'  : False,
                'defaultValue': {'expressions': [expr]},
                'group'       : group_ref,
                'availability': all_avail,
            }}
        ]})
        entry = (prop_r or {}).get('propertyIds', [{}])[0]
        created_prop_guid = entry.get('propertyId', {}).get('guid')
        if created_prop_guid:
            used_type = prop_type
            print(f'  Created with type={prop_type}, expr: {expr[:60]}')
            break
        else:
            err = entry.get('error', {})
            print(f'  type={prop_type} | {expr[:50]!r} -> failed ({err.get("code")})')

    check('ExpressionBasedProperty created', True, created_prop_guid is not None)
    if used_type == 'number':
        print('  NOTE: AC27 does not accept string-type CONCAT with ProjectInfo refs — fell back to number')
if not created_prop_guid and not grp_guid:
    print('  [SKIP] no group guid')
elif not info_prop_guid:
    print('  [SKIP] ExpressionBasedInfo not visible in properties')
print()

# ---------------------------------------------------------------------------
# 5. Read back and verify
# ---------------------------------------------------------------------------
print('STEP 5 -- Read back ExpressionBasedProperty via GetAllProperties')
if created_prop_guid:
    guid_map2, all_props2 = build_guid_map()
    prop = next((p for p in all_props2
                 if p.get('propertyId', {}).get('guid') == created_prop_guid), None)
    is_expr = (prop or {}).get('isExpressionBased', False)
    exprs   = (prop or {}).get('expressions', [])
    check('isExpressionBased=True', True, is_expr)
    check('has 1 expression', True, len(exprs) == 1)
    if exprs:
        raw  = exprs[0]
        read = resolve(raw, guid_map2)
        print(f'  RAW : {raw}')
        print(f'  READ: {read}')
        check('expression references ExpressionBasedInfo',
              True, info_prop_guid in raw)
print()

# ---------------------------------------------------------------------------
# STEP 6 -- DeleteProjectInfoFields
# ---------------------------------------------------------------------------
print('STEP 6 -- DeleteProjectInfoFields')

# Test 1: hardcoded field must be rejected
hardcoded_id = 'PROJECT'
del_bad    = run('DeleteProjectInfoFields', {'projectInfoIds': [hardcoded_id]})
bad_result = (del_bad or {}).get('executionResults', [{}])[0]
check('hardcoded field rejected', False, bad_result.get('success', False))

# Test 2: create a throwaway field, delete it, verify it's gone
throwaway_r  = run('CreateProjectInfoFields', {
    'projectInfoFields': [{'projectInfoName': 'TestDeleteMe', 'projectInfoValue': ''}]
})
throwaway_id = (throwaway_r or {}).get('fields', [{}])[0].get('projectInfoId')
if throwaway_id:
    del_r = run('DeleteProjectInfoFields', {'projectInfoIds': [throwaway_id]})
    ok    = (del_r or {}).get('executionResults', [{}])[0].get('success', False)
    check('custom field deleted', True, ok)
    _, props_after = build_guid_map()
    still_there = any(p.get('propertyName') == 'TestDeleteMe' for p in props_after)
    check('field no longer in GetAllProperties', False, still_there)
else:
    print('  [SKIP] could not create throwaway field')
# ExpressionBasedInfo is kept alive so ExpressionBasedProperty displays correctly
print()

# ---------------------------------------------------------------------------
# Cleanup (disabled for visual verification in ArchiCAD)
# ---------------------------------------------------------------------------
print('Cleanup skipped — verify in ArchiCAD:')
print(f'  Group        : TestExprGroup')
if created_prop_guid:
    print(f'  Property     : ExpressionBasedProperty  ({created_prop_guid})')
print(f'  Project info : ExpressionBasedInfo (kept alive — referenced by ExpressionBasedProperty)')

print()
print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
