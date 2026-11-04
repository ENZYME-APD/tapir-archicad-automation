"""
Manual verification for issue #895: GetDetailsOfElements now returns 'reveal' and
'revealDepthOffset' for windows and doors - the same two values ModifyWindows and
ModifyDoors write (#581) - so a script can read its own write back.

The script creates a wall with one window and one door, writes a reveal with a
known depth offset through ModifyWindows / ModifyDoors, reads it back through
GetDetailsOfElements and checks the round trip is identical, for the reveal
turned on and turned off. Everything it created is deleted at the end.

Not part of the auto-discovered Examples/ (see test_examples.py +
ExpectedOutputs/): TestProject.pla has no windows or doors, and this script
exits non-zero on a real failure, which would abort that harness's whole run
rather than just failing this one script.
Run manually: python manual_tests/test_window_door_reveal_round_trip.py
(from the Examples/ folder, with Archicad and the Tapir Add-On running).
"""
import os
import sys

# aclib lives in the parent Examples/ folder - this script is one level below it precisely
# so it stays out of Examples/'s own flat, non-recursive test auto-discovery.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

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

def details_of(elementId):
    return run('GetDetailsOfElements', {'elements': [{'elementId': elementId}]})['detailsOfElements'][0]['details']

# 1) A wall with one window and one door, from the project's tool defaults.
wallId = run('CreateWalls', {
    'wallsData': [{
        'begCoordinate': {'x': 0.0, 'y': 0.0},
        'endCoordinate': {'x': 8.0, 'y': 0.0},
        'height': 3.0,
        'thickness': 0.4
    }]
})['elements'][0]['elementId']

windowId = run('CreateWindows', {
    'windowsData': [{'ownerWallId': wallId, 'centerOffset': 2.0, 'width': 1.0, 'height': 1.5}]
})['elements'][0]['elementId']

doorId = run('CreateDoors', {
    'doorsData': [{'ownerWallId': wallId, 'centerOffset': 5.0, 'width': 1.0, 'height': 2.1}]
})['elements'][0]['elementId']

# 2) The details of a fresh opening carry both reveal fields.
for label, elementId in (('window', windowId), ('door', doorId)):
    details = details_of(elementId)
    check('{} details contain reveal'.format(label), 'reveal' in details, str(details))
    check('{} details contain revealDepthOffset'.format(label), 'revealDepthOffset' in details, str(details))
    check('{} reveal is a boolean'.format(label), isinstance(details.get('reveal'), bool), str(details.get('reveal')))

# 3) ModifyWindows / ModifyDoors -> GetDetailsOfElements round trip, reveal on and off.
for label, elementId, cmd, key in (('window', windowId, 'ModifyWindows', 'windowsWithDetails'),
                                   ('door', doorId, 'ModifyDoors', 'doorsWithDetails')):
    for reveal, depthOffset in ((True, 0.1), (True, 0.0), (False, 0.05)):
        result = run(cmd, {key: [{'elementId': elementId, 'reveal': reveal, 'revealDepthOffset': depthOffset}]})['executionResults'][0]
        check('{} {} with reveal={} depthOffset={} succeeds'.format(cmd, label, reveal, depthOffset),
              result['success'] is True, str(result))
        details = details_of(elementId)
        check('{} reads back reveal={}'.format(label, reveal), details['reveal'] is reveal, str(details['reveal']))
        check('{} reads back revealDepthOffset={}'.format(label, depthOffset),
              abs(details['revealDepthOffset'] - depthOffset) < 1e-6, str(details['revealDepthOffset']))

# 4) Clean up: deleting the wall deletes its openings with it.
run('DeleteElements', {'elements': [{'elementId': wallId}]})

print('{} passed, {} failed'.format(passes, fails))
sys.exit(0 if fails == 0 else 1)
