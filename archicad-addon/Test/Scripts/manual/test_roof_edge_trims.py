"""
Manual verification for issue #2363: GetDetailsOfElements reports the cut of each
edge of a single-plane roof's polygon as edgeTrims, and ModifyRoofs writes it.

The script creates a single-plane roof, reads its edge trims, sets the second edge
to a custom cut angle and reads the trims back. It checks that the edge got the new
mode and angle and that the other edges, their side surfaces, the roof's angle and
its pivot line stayed as they were. It also checks that a wrong item count and a
combination with polygonOutline are refused. The roof is deleted at the end.

Run manually, with Archicad and the Tapir Add-On running:
python archicad-addon/Test/Scripts/manual/test_roof_edge_trims.py
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

passes = fails = 0


def check(label, ok, detail=''):
    global passes, fails
    if ok:
        passes += 1
        print('PASS', label)
    else:
        fails += 1
        print('FAIL', label, detail)


def details(roof):
    return aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [roof]}, debug=False)['detailsOfElements'][0]['details']


roof = aclib.RunTapirCommand('CreateRoofs', {'roofsData': [{
    'level': 3.0,
    'polygonCoordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}, {'x': 6.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}],
    'pivotLine': {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 6.0, 'y': 0.0}},
    'angle': math.radians(20)
}]}, debug=False)['elements'][0]

try:
    before = details(roof)
    trims = before.get('edgeTrims')
    check('edgeTrims reported, one per edge', trims is not None and len(trims) == 4, before.get('edgeTrims'))

    customAngle = math.radians(60)
    result = aclib.RunTapirCommand('ModifyRoofs', {'roofsWithDetails': [{
        'elementId': roof['elementId'],
        'edgeTrims': [{}, {'trimType': 'CustomAngle', 'angle': customAngle}, {}, {}]
    }]}, debug=False)
    check('ModifyRoofs succeeds', result['executionResults'][0]['success'], result)

    after = details(roof)
    newTrims = after.get('edgeTrims') or []
    check('edge 1 is CustomAngle', len(newTrims) == 4 and newTrims[1]['trimType'] == 'CustomAngle', newTrims)
    check('edge 1 has the new angle', len(newTrims) == 4 and abs(newTrims[1]['angle'] - customAngle) < 1e-6, newTrims)
    check('other edges unchanged', trims is not None and len(newTrims) == 4 and all(newTrims[i] == trims[i] for i in (0, 2, 3)), (trims, newTrims))
    check('side surfaces unchanged', after.get('edgeSideMaterials') == before.get('edgeSideMaterials'))
    check('roof angle unchanged', abs(after['angle'] - before['angle']) < 1e-9, (before['angle'], after['angle']))
    check('pivot line unchanged', after['pivotLine'] == before['pivotLine'], (before['pivotLine'], after['pivotLine']))

    result = aclib.RunTapirCommand('ModifyRoofs', {'roofsWithDetails': [{
        'elementId': roof['elementId'],
        'edgeTrims': [{'trimType': 'Horizontal'}]
    }]}, debug=False)
    check('wrong item count refused', not result['executionResults'][0]['success'], result)

    result = aclib.RunTapirCommand('ModifyRoofs', {'roofsWithDetails': [{
        'elementId': roof['elementId'],
        'edgeTrims': [{}, {}, {}, {}],
        'polygonOutline': [{'x': 0.0, 'y': 0.0}, {'x': 7.0, 'y': 0.0}, {'x': 7.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}]
    }]}, debug=False)
    check('combination with polygonOutline refused', not result['executionResults'][0]['success'], result)
finally:
    aclib.RunTapirCommand('DeleteElements', {'elements': [roof]}, debug=False)

print(f'{passes} passed, {fails} failed')
sys.exit(1 if fails else 0)
