"""
Live test: read and write the fill orientation (origin, type, distortion) of Hatches through
GetDetailsOfElements / SetDetailsOfElements, and the origin of a Slab's floor plan fill
(issue #662). Creates its own elements on the current story and deletes them at the end.
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


def details(guid):
    return run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']


def set_details(guid, typeSpecificDetails):
    r = run('SetDetailsOfElements', {'elementsWithDetails': [{
        'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': typeSpecificDetails}}]})
    check('SetDetailsOfElements succeeded', True, r['executionResults'][0]['success'])


try:
    print('HATCH -- fillOrientation is returned')
    r = run('CreateHatches', {'hatchesData': [{
        'coordinates': [{'x': 300, 'y': 300}, {'x': 310, 'y': 300}, {'x': 310, 'y': 310}, {'x': 300, 'y': 310}]}]})
    hatch = r['elements'][0]['elementId']['guid']
    created.append(hatch)
    orientation = details(hatch)['typeSpecificDetails']['fillOrientation']
    print('  after create:', orientation)
    check('fillOrientation has a type', True, 'type' in orientation)
    check('fillOrientation has an origin', True, 'origin' in orientation)

    print('HATCH -- origin only: detaches from the project origin, Global becomes Rotated')
    set_details(hatch, {'fillOrientation': {'origin': {'x': 301.25, 'y': 302.5}}})
    orientation = details(hatch)['typeSpecificDetails']['fillOrientation']
    check('origin.x', 301.25, orientation['origin']['x'])
    check('origin.y', 302.5, orientation['origin']['y'])
    check('type', 'Rotated', orientation['type'])

    print('HATCH -- distortion matrix, origin kept')
    set_details(hatch, {'fillOrientation': {'type': 'Distorted', 'matrix00': 0.0, 'matrix10': 1.0, 'matrix01': -1.0, 'matrix11': 0.0}})
    orientation = details(hatch)['typeSpecificDetails']['fillOrientation']
    check('type', 'Distorted', orientation['type'])
    check('matrix10', 1.0, orientation['matrix10'])
    check('matrix01', -1.0, orientation['matrix01'])
    check('origin.x kept', 301.25, orientation['origin']['x'])

    print('SLAB -- floorFill.orientation.origin (issue #662)')
    r = run('CreateSlabs', {'slabsData': [{
        'level': 0, 'polygonCoordinates': [{'x': 320, 'y': 300}, {'x': 330, 'y': 300}, {'x': 330, 'y': 310}, {'x': 320, 'y': 310}]}]})
    slab = r['elements'][0]['elementId']['guid']
    created.append(slab)
    r = run('ModifySlabs', {'slabsWithDetails': [{
        'elementId': {'guid': slab}, 'floorFill': {'orientation': {'origin': {'x': 321.5, 'y': 303.0}}}}]})
    print('  ModifySlabs:', r)
    orientation = details(slab)['typeSpecificDetails']['floorFill']['orientation']
    check('slab origin.x', 321.5, orientation['origin']['x'])
    check('slab origin.y', 303.0, orientation['origin']['y'])
finally:
    if created:
        run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created]})

print(f'\n{passes} passed, {fails} failed')
