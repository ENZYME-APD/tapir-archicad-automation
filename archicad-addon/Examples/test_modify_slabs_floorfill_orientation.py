"""
Live test for ModifySlabs floorFill.orientation (issue #662): a partial orientation object
merges into the slab's current hatch orientation instead of replacing it with a zero-initialized
one (previously sending only 'origin' silently reset 'type' back to Global). Of the orientation,
Archicad applies only the type to a Slab through the API and resets the origin and the matrix,
so ModifySlabs reads the slab back and reports a given origin or matrix that did not stick as a
failure; the other details of the call are still applied.
"""
import aclib

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0
created_guids = []

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected!r}, got={got!r})')

def get_floor_fill(guid):
    d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})
    return d['detailsOfElements'][0]['details']['floorFill']

def modify_floor_fill(guid, floor_fill):
    r = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid}, 'floorFill': floor_fill}]})
    return r['executionResults'][0]

def cleanup():
    if created_guids:
        run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created_guids]})

# =============================================================================
print('TEST -- issue #662: partial floorFill.orientation must merge, not replace')
# =============================================================================
outline = [{'x': 0, 'y': 0}, {'x': 10, 'y': 0}, {'x': 10, 'y': 10}, {'x': 0, 'y': 10}]
r = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)

ff = get_floor_fill(guid)
print(f"  [INFO] initial orientation type: {ff['orientation']['type']}")

print('Step 1 -- set type Rotated')
result = modify_floor_fill(guid, {'use': True, 'orientation': {'type': 'Rotated'}})
check('modify succeeds', True, result.get('success'))
ff = get_floor_fill(guid)
check('type became Rotated', 'Rotated', ff['orientation']['type'])

print('Step 2 -- send only origin: the type must survive, the ignored origin is reported')
result = modify_floor_fill(guid, {'orientation': {'origin': {'x': 2.75, 'y': 0.15}}})
check('modify reports the ignored origin', (False, True), (result.get('success'), 'origin' in result.get('error', {}).get('message', '')))
ff = get_floor_fill(guid)
check('type still Rotated after origin-only call', 'Rotated', ff['orientation']['type'])
check('floorFill.use survived orientation-only call', True, ff['use'])

print('Step 3 -- a distortion matrix is reported as ignored too')
result = modify_floor_fill(guid, {'orientation': {'matrix00': 0.0, 'matrix10': 1.0, 'matrix01': -1.0, 'matrix11': 0.0}})
check('modify reports the ignored matrix', (False, True), (result.get('success'), 'matrix10' in result.get('error', {}).get('message', '')))
print()

cleanup()

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    import sys
    sys.exit(1)
