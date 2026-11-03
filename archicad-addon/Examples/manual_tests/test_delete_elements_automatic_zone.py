"""
Manual verification for issue #901: DeleteElements with an automatic zone and
its four bounding walls in ONE call, walls first, made Archicad open the modal
"No closed polygon found around the clicked point" dialog. The walls were
deleted before the zone, the zone was recomputed from a boundary that was no
longer closed, and the command - and every later Tapir call - blocked until
someone clicked OK in Archicad. The command now deletes the zones of a call
before everything else, in a delete call of their own within the same undoable
command, so the input order no longer matters.

The script builds a closed room of four walls far from the origin, places an
automatic zone in it and deletes all five elements in one call with the walls
first; then it builds the same room again and deletes with the zone first. Both
calls must return without a dialog, with every execution result successful and
every element gone. Without the fix the first delete does not return until the
dialog is clicked in Archicad, so watch Archicad while this runs.

An automatic zone is found on the story that is active in the floor plan, and
the walls are created on the story of the Wall tool defaults, so run this with
the floor plan open on the current story.

Not part of the auto-discovered Examples/ (see test_examples.py +
ExpectedOutputs/): it needs an empty spot in the project for the room, it may
block on the dialog it tests for, and it exits non-zero on a real failure,
which would abort that harness's whole run rather than just failing this one
script.
Run manually: python manual_tests/test_delete_elements_automatic_zone.py
(from the Examples/ folder, with Archicad and the Tapir Add-On running).
"""
import os
import sys

# aclib lives in the parent Examples/ folder - this script is one level below it precisely
# so it stays out of Examples/'s own flat, non-recursive test auto-discovery.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

# Far from anything the open project is likely to contain: the automatic zone
# needs a closed room with nothing else in it.
ORIGIN_X = 200.0
ORIGIN_Y = 200.0
ROOM_SIZE = 6.0

passes = fails = 0

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

def check(label, ok, detail=''):
    global passes, fails
    if ok:
        passes += 1
        print('PASS: ' + label)
    else:
        fails += 1
        print('FAIL: {} ({})'.format(label, detail))

def is_gone(elementId):
    details = run('GetDetailsOfElements', {
        'elements': [{'elementId': elementId}]
    })['detailsOfElements'][0]
    return 'error' in details

def build_room():
    """Four walls whose reference lines close a square, and an automatic zone found in it.
    Returns (wallIds, zoneId); exits when the zone cannot be found, as nothing can be tested then."""
    corners = [(0.0, 0.0), (ROOM_SIZE, 0.0), (ROOM_SIZE, ROOM_SIZE), (0.0, ROOM_SIZE)]
    wallsData = []
    for i in range(4):
        bx, by = corners[i]
        ex, ey = corners[(i + 1) % 4]
        wallsData.append({
            'begCoordinate': {'x': ORIGIN_X + bx, 'y': ORIGIN_Y + by},
            'endCoordinate': {'x': ORIGIN_X + ex, 'y': ORIGIN_Y + ey},
            'height': 3.0,
            'thickness': 0.3
        })
    wallIds = [w['elementId'] for w in run('CreateWalls', {'wallsData': wallsData})['elements']]

    zone = run('CreateZones', {
        'zonesData': [{
            'name': 'Tapir DeleteElements ZoneTest',
            'numberStr': '901',
            'geometry': {
                'referencePosition': {'x': ORIGIN_X + ROOM_SIZE / 2, 'y': ORIGIN_Y + ROOM_SIZE / 2}
            }
        }]
    })['elements'][0]
    if 'elementId' not in zone:
        print('Could not place the automatic zone, so there is nothing to test: {}'.format(zone))
        print('Is the floor plan open on the current story, and is the room around '
              '({}, {}) empty?'.format(ORIGIN_X, ORIGIN_Y))
        # The zone-first order is the one that always worked, so the walls can be cleaned up.
        run('DeleteElements', {'elements': [{'elementId': wallId} for wallId in wallIds]})
        sys.exit(1)
    return wallIds, zone['elementId']

def delete_in_one_call(label, elementIds):
    result = run('DeleteElements', {
        'elements': [{'elementId': elementId} for elementId in elementIds]
    })
    results = result['executionResults']
    check(label + ': one execution result per input element',
          len(results) == len(elementIds), str(result))
    check(label + ': every execution result is successful',
          all(r['success'] is True for r in results), str(result))
    survivors = [elementId for elementId in elementIds if not is_gone(elementId)]
    check(label + ': every element is gone', len(survivors) == 0, str(survivors))
    if survivors:
        # Clean up in the order that always worked: the zone (if any) comes first already,
        # as the survivors keep the input order of this call.
        run('DeleteElements', {'elements': [{'elementId': elementId} for elementId in survivors]})

# 1) The reporter's case: the walls before the zone, as GetElementsByType lists them.
wallIds, zoneId = build_room()
delete_in_one_call('walls first', wallIds + [zoneId])

# 2) The zone before the walls: the order that worked before the fix must still work.
wallIds, zoneId = build_room()
delete_in_one_call('zone first', [zoneId] + wallIds)

print('{} passed, {} failed'.format(passes, fails))
sys.exit(0 if fails == 0 else 1)
