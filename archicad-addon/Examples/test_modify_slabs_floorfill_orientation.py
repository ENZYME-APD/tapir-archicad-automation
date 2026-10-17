"""
Live test for ModifySlabs floorFill.orientation merge semantics (issue #662): a partial
orientation object must merge into the slab's current hatch orientation instead of replacing
it with a zero-initialized one (previously sending only 'origin' silently reset 'type' back
to Global and the matrix to zeros). Also reports whether 'origin' itself is applied by
ACAPI_Element_Change - suspected to be ignored by the Archicad core regardless of Tapir.
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
    return r['executionResults'][0].get('success')

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
check('initial orientation type is Global', 'Global', ff['orientation']['type'])

print('Step 1 -- set type Rotated with an explicit matrix')
ok = modify_floor_fill(guid, {'use': True, 'orientation': {
    'type': 'Rotated', 'matrix00': 0.0, 'matrix10': 1.0, 'matrix01': -1.0, 'matrix11': 0.0}})
check('modify succeeds', True, ok)
ff = get_floor_fill(guid)
check('type became Rotated', 'Rotated', ff['orientation']['type'])
check('matrix applied', (0.0, 1.0, -1.0, 0.0),
      (ff['orientation']['matrix00'], ff['orientation']['matrix10'],
       ff['orientation']['matrix01'], ff['orientation']['matrix11']))

print('Step 2 -- send only origin: type and matrix must survive (previously reset to Global/zeros)')
ok = modify_floor_fill(guid, {'orientation': {'origin': {'x': 2.75, 'y': 0.15}}})
check('modify succeeds', True, ok)
ff = get_floor_fill(guid)
check('type still Rotated after origin-only call', 'Rotated', ff['orientation']['type'])
check('matrix survived origin-only call', (0.0, 1.0, -1.0, 0.0),
      (ff['orientation']['matrix00'], ff['orientation']['matrix10'],
       ff['orientation']['matrix01'], ff['orientation']['matrix11']))
check('floorFill.use survived orientation-only call', True, ff['use'])

# Informational only: whether ACAPI_Element_Change honors hatchOrientation.origo at all is a
# separate, suspected core-API limitation (issue #662) - do not fail the test on it.
origin = ff['orientation']['origin']
applied = (round(origin['x'], 6), round(origin['y'], 6)) == (2.75, 0.15)
print(f"  [INFO] origin write-through by ACAPI_Element_Change: {'applied' if applied else 'ignored by Archicad'}  (got={origin!r})")
print()

cleanup()

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    import sys
    sys.exit(1)
