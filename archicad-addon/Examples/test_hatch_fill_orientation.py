"""
Live test: read and write the fill orientation (origin, type, distortion) of Hatches through
GetDetailsOfElements / SetDetailsOfElements. Creates its own hatch on the current story with a
Vector fill (Archicad ignores the orientation of Solid fills) and deletes it at the end.
"""
import aclib


def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)


passes = fails = 0
created = []


def check(label, expected, got, tol=1e-6):
    global passes, fails
    if isinstance(expected, float):
        ok = got is not None and abs(got - expected) < tol
    else:
        ok = got == expected
    if ok:
        passes += 1
    else:
        fails += 1
    print(f"  [{'PASS' if ok else 'FAIL'}] {label}  (expected={expected!r}, got={got!r})")


def orientation_of(guid):
    return run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']['fillOrientation']


def set_orientation(guid, fillOrientation):
    r = run('SetDetailsOfElements', {'elementsWithDetails': [{
        'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {'fillOrientation': fillOrientation}}}]})
    check('SetDetailsOfElements succeeded', True, r['executionResults'][0]['success'])


fills = run('GetAttributesByType', {'attributeType': 'Fill'})['attributes']
fillData = run('GetFills', {'attributeIds': [{'attributeId': f['attributeId']} for f in fills], 'fields': ['subType']})['fills']
vector = next(f for f in fillData if f.get('subType') == 'Vector')
print(f"Using the Vector fill '{vector['name']}'")

try:
    print('HATCH -- fillOrientation is returned')
    r = run('CreateHatches', {'hatchesData': [{
        'coordinates': [{'x': 300, 'y': 300}, {'x': 310, 'y': 300}, {'x': 310, 'y': 310}, {'x': 300, 'y': 310}],
        'fillId': vector['attributeId']}]})
    hatch = r['elements'][0]['elementId']['guid']
    created.append(hatch)
    orientation = orientation_of(hatch)
    print('  after create:', orientation)
    check('fillOrientation has a type', True, 'type' in orientation)
    check('fillOrientation has an origin', True, 'origin' in orientation)

    print('HATCH -- origin only: detaches from the project origin, Global becomes Rotated')
    set_orientation(hatch, {'origin': {'x': 301.25, 'y': 302.5}})
    orientation = orientation_of(hatch)
    check('origin.x', 301.25, orientation['origin']['x'])
    check('origin.y', 302.5, orientation['origin']['y'])
    check('type', 'Rotated', orientation['type'])

    print('HATCH -- distortion matrix, origin kept')
    set_orientation(hatch, {'type': 'Distorted', 'matrix00': 0.0, 'matrix10': 1.0, 'matrix01': -1.0, 'matrix11': 0.0})
    orientation = orientation_of(hatch)
    check('type', 'Distorted', orientation['type'])
    check('matrix10', 1.0, orientation['matrix10'])
    check('matrix01', -1.0, orientation['matrix01'])
    check('origin.x kept', 301.25, orientation['origin']['x'])
finally:
    if created:
        run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created]})

print(f'\n{passes} passed, {fails} failed')
