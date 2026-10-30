"""
Example for issue #1598: CreateWalls and ModifyWalls take 'flipped', which mirrors the
wall to its reference line. Without it a new wall takes the side of the Wall tool default,
so a wall read with GetDetailsOfElements could not be rebuilt as it was.

The script creates one wall with flipped false and one with flipped true, flips the first
one with ModifyWalls, reads them back, prints what matched, and cleans up after itself.
"""
import aclib

def run (command, parameters = None):
    return aclib.RunTapirCommand (command, parameters or {}, debug = False)

passes = fails = 0

def check (label, expected, got):
    global passes, fails
    ok = (got == expected)
    if ok:
        passes += 1
    else:
        fails += 1
    print ('  [{}] {}  (expected={!r}, got={!r})'.format ('PASS' if ok else 'FAIL', label, expected, got))

def wallDetails (elementId):
    return run ('GetDetailsOfElements', {'elements': [{'elementId': elementId}]})['detailsOfElements'][0]['details']

created = []

print ('CreateWalls with flipped false and flipped true')
response = run ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 6000.0, 'y': 0.0},
    'endCoordinate': {'x': 6005.0, 'y': 0.0},
    'height': 3.0,
    'thickness': 0.3,
    'structureType': 'Basic',
    'referenceLineLocation': 'Outside',
    'flipped': False
}, {
    'begCoordinate': {'x': 6000.0, 'y': 5.0},
    'endCoordinate': {'x': 6005.0, 'y': 5.0},
    'height': 3.0,
    'thickness': 0.3,
    'structureType': 'Basic',
    'referenceLineLocation': 'Outside',
    'flipped': True
}]})
for element in response['elements']:
    check ('wall created', True, 'elementId' in element)
    if 'elementId' in element:
        created.append (element['elementId'])

if len (created) == 2:
    check ('flipped false applied', False, wallDetails (created[0])['flipped'])
    check ('flipped true applied', True, wallDetails (created[1])['flipped'])

    print ('ModifyWalls flips the first wall')
    run ('ModifyWalls', {'wallsWithDetails': [{'elementId': created[0], 'flipped': True}]})
    d = wallDetails (created[0])
    check ('flipped after ModifyWalls', True, d['flipped'])
    check ('reference line unchanged', {'x': 6000.0, 'y': 0.0}, d['begCoordinate'])

run ('DeleteElements', {'elements': [{'elementId': elementId} for elementId in created]})

print ('{} passed, {} failed'.format (passes, fails))
