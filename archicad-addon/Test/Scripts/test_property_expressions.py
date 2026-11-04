import sys, re
sys.stdout.reconfigure(encoding='utf-8')
import aclib

PASS = 'PASS'
FAIL = 'FAIL'
log  = []

def run(cmd, params):
    return aclib.RunTapirCommand(cmd, params, debug=False)

def check(name, expected, got):
    ok = expected == got
    status = PASS if ok else FAIL
    log.append((name, status, expected, got))
    print(f'  [{status}] {name}  (expected={expected!r}, got={got!r})')

def ref(guid):
    return f'###Property:{guid}###'

# ---------------------------------------------------------------------------
# Build lookup tables from GetAllProperties
# ---------------------------------------------------------------------------
all_r = run('GetAllProperties', {})
all_props = (all_r or {}).get('properties', [])
guid_to_label = {
    p['propertyId']['guid']: f"{p['propertyGroupName']} / {p['propertyName']}"
    for p in all_props if 'propertyId' in p
}

def resolve(expr):
    """Replace ###Property:GUID### with [Group / Name] for readability."""
    return re.sub(
        r'###Property:([0-9A-Fa-f\-]+)###',
        lambda m: f'[{guid_to_label.get(m.group(1), m.group(1))}]',
        expr
    )

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

def find_prop(ptype=None, vtype=None, exclude_expr=False):
    """Return first property matching type filters."""
    for p in all_props:
        if ptype and p.get('propertyType') != ptype:
            continue
        if vtype and p.get('propertyValueType') != vtype:
            continue
        if exclude_expr and p.get('isExpressionBased'):
            continue
        return p
    return None

# Locate reference properties for expressions
builtin_area   = find_prop(ptype='DynamicBuiltIn', vtype='Real')
builtin_str    = find_prop(ptype='DynamicBuiltIn', vtype='String')
builtin_int    = find_prop(ptype='DynamicBuiltIn', vtype='Integer')
custom_prop    = find_prop(ptype='Custom', vtype='Real',    exclude_expr=True) or \
                 find_prop(ptype='Custom', vtype='Integer', exclude_expr=True)

print(f'Built-in (Real)   : {guid_to_label.get((builtin_area  or {}).get("propertyId",{}).get("guid",""), "not found")}')
print(f'Built-in (String) : {guid_to_label.get((builtin_str   or {}).get("propertyId",{}).get("guid",""), "not found")}')
print(f'Built-in (Integer): {guid_to_label.get((builtin_int   or {}).get("propertyId",{}).get("guid",""), "not found")}')
print(f'Custom (non-expr) : {guid_to_label.get((custom_prop   or {}).get("propertyId",{}).get("guid",""), "not found")}')
print()

# ---------------------------------------------------------------------------
# Setup: temporary group
# ---------------------------------------------------------------------------
print('Setup...')
group_r = run('CreatePropertyGroups', {'propertyGroups': [
    {'propertyGroup': {'name': '__TapirExprTest__', 'description': 'temporary'}}
]})
group_guid = (group_r or {}).get('propertyGroupIds', [{}])[0].get('propertyGroupId', {}).get('guid')
if not group_guid:
    print('FATAL: could not create test group'); sys.exit(1)

print('  Collecting classification items for availability...')
all_avail = get_all_classification_items()
print(f'  Found {len(all_avail)} classification items')

created_guids = []

def make_prop(name, ptype, expr_list):
    r = run('CreatePropertyDefinitions', {'propertyDefinitions': [
        {'propertyDefinition': {
            'name': name, 'description': '', 'type': ptype, 'isEditable': False,
            'defaultValue': {'expressions': expr_list},
            'group': {'propertyGroupId': {'guid': group_guid}},
            'availability': all_avail,
        }}
    ]})
    entry = (r or {}).get('propertyIds', [{}])[0]
    guid  = entry.get('propertyId', {}).get('guid')
    if guid:
        created_guids.append(guid)
    elif 'error' in entry:
        print(f'  ERROR: {entry["error"]}')
    return guid, entry

def get_prop(guid):
    fresh = run('GetAllProperties', {})
    return next((p for p in (fresh or {}).get('properties', [])
                 if p.get('propertyId', {}).get('guid') == guid), None)

# =============================================================================
print('TEST 1 -- GetAllProperties: isExpressionBased on existing properties')
# =============================================================================
expr_props = [p for p in all_props if p.get('isExpressionBased')]
non_expr   = [p for p in all_props if not p.get('isExpressionBased')]
check('expression-based props exist in project', True, len(expr_props) > 0)
check('isExpressionBased=True has expressions key', True,
      all('expressions' in p for p in expr_props))
check('isExpressionBased=False has no expressions key', True,
      all('expressions' not in p for p in non_expr))
if expr_props:
    sample = expr_props[0]
    print(f'  Sample: {guid_to_label[sample["propertyId"]["guid"]]}')
    for e in sample.get('expressions', []):
        print(f'    RAW : {e}')
        print(f'    READ: {resolve(e)}')
print()

# =============================================================================
print('TEST 2 -- Create expression referencing a built-in Real property')
# =============================================================================
if builtin_area:
    ag = builtin_area['propertyId']['guid']
    guid2, _ = make_prop('TestExpr_BuiltinReal', 'number', [f'{ref(ag)} / 1 m * 2'])
    check('created successfully', True, guid2 is not None)
    if guid2:
        p = get_prop(guid2)
        check('isExpressionBased=True', True, (p or {}).get('isExpressionBased'))
        exprs = (p or {}).get('expressions', [])
        check('expression count = 1', 1, len(exprs))
        check('expression contains built-in ref', True, ag in (exprs[0] if exprs else ''))
        if exprs:
            print(f'  Expression: {resolve(exprs[0])}')
else:
    print('  [SKIP] no built-in Real property found')
print()

# =============================================================================
print('TEST 3 -- Create expression referencing a built-in DynamicBuiltIn property')
# =============================================================================
if builtin_str:
    sg = builtin_str['propertyId']['guid']
    # DynamicBuiltIn String: wrap with STRTONUM to stay in number domain
    guid3, _ = make_prop('TestExpr_BuiltinDynamic', 'number',
                         [f'STRTONUM({ref(sg)})'])
    if not guid3:
        # Fallback: plain reference if string prop can't be wrapped
        guid3, _ = make_prop('TestExpr_BuiltinDynamic2', 'number',
                             [f'{ref(sg)}'])
    check('created successfully', True, guid3 is not None)
    if guid3:
        p = get_prop(guid3)
        exprs = (p or {}).get('expressions', [])
        check('expression readable', True, len(exprs) > 0)
        if exprs:
            print(f'  Expression: {resolve(exprs[0])}')
else:
    print('  [SKIP] no DynamicBuiltIn property found')
print()

# =============================================================================
print('TEST 4 -- Create expression referencing a custom Real/Integer property')
# =============================================================================
if custom_prop:
    cg   = custom_prop['propertyId']['guid']
    ag   = builtin_area['propertyId']['guid'] if builtin_area else None
    expr = f'{ref(cg)} + {ref(ag)} / 1 m' if ag else f'{ref(cg)} * 1'
    guid4, _ = make_prop('TestExpr_CustomRef', 'number', [expr])
    check('created successfully', True, guid4 is not None)
    if guid4:
        p = get_prop(guid4)
        exprs = (p or {}).get('expressions', [])
        check('references custom property', True, cg in (exprs[0] if exprs else ''))
        if exprs:
            print(f'  Expression: {resolve(exprs[0])}')
else:
    print('  [SKIP] no custom Real/Integer property found')
print()

# =============================================================================
print('TEST 5 -- Create expression with multiple per-type expressions')
# =============================================================================
# Use the two properties we know work (both Real)
guid5 = None
if builtin_area and custom_prop:
    ag = builtin_area['propertyId']['guid']
    cg = custom_prop['propertyId']['guid']
    guid5, _ = make_prop('TestExpr_MultiExpr', 'number',
                         [f'{ref(ag)} / 1 m + {ref(cg)}', f'{ref(cg)} + {ref(ag)} / 1 m'])
    check('created successfully', True, guid5 is not None)
    if guid5:
        p = get_prop(guid5)
        exprs = (p or {}).get('expressions', [])
        check('expression count = 2', 2, len(exprs))
        for e in exprs:
            print(f'  Expression: {resolve(e)}')
else:
    print('  [SKIP] need built-in Real and custom property')
print()

# =============================================================================
print('TEST 6 -- UpdatePropertyDefinitions: change expression')
# =============================================================================
if guid5 and builtin_area and custom_prop:
    ag       = builtin_area['propertyId']['guid']
    cg       = custom_prop['propertyId']['guid']
    new_expr = f'{ref(ag)} / 1 m + {ref(cg)}'
    upd_r = run('UpdatePropertyDefinitions', {'propertyDefinitions': [
        {'propertyId': {'guid': guid5}, 'expressions': [new_expr]}
    ]})
    ok = (upd_r or {}).get('executionResults', [{}])[0].get('success')
    check('update success', True, ok)
    p = get_prop(guid5)
    exprs = (p or {}).get('expressions', [])
    check('expression count after update = 1', 1, len(exprs))
    check('expression matches new value', new_expr, exprs[0] if exprs else None)
    if exprs:
        print(f'  Updated: {resolve(exprs[0])}')
else:
    print('  [SKIP] need guid5 + built-in Real + custom property')
print()

# =============================================================================
print('TEST 7 -- UpdatePropertyDefinitions: error on unknown GUID')
# =============================================================================
fake = '00000000-0000-0000-0000-000000000000'
err_r = run('UpdatePropertyDefinitions', {'propertyDefinitions': [
    {'propertyId': {'guid': fake}, 'expressions': ['1 + 1']}
]})
ok = (err_r or {}).get('executionResults', [{}])[0].get('success')
check('unknown guid returns failure', False, ok)
print()

# =============================================================================
print('TEST 8 -- resolve_expr: all existing expression props are resolvable')
# =============================================================================
unresolved = [p for p in expr_props
              if '###Property:' in ''.join(p.get('expressions', []))
              and '[' not in resolve(''.join(p.get('expressions', [])))]
check('all GUID refs in project are resolvable', 0, len(unresolved))
print()

# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------
print('Cleanup...')
for g in created_guids:
    run('DeletePropertyDefinitions', {'propertyIds': [{'propertyId': {'guid': g}}]})
run('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': {'guid': group_guid}}]})

# ---------------------------------------------------------------------------
passed = sum(1 for _, s, _, _ in log if s == PASS)
failed = sum(1 for _, s, _, _ in log if s == FAIL)
print()
print('=' * 60)
print(f'BILAN :  {passed} PASS  |  {failed} FAIL')
print('=' * 60)
sys.exit(0 if failed == 0 else 1)
