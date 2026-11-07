"""
Manual verification for issue #555: MoveElements and RotateElements used to
answer {"success": true} for an element on a hidden or locked layer and leave
it where it was, because ACAPI_Element_Edit reports NoError for an element it
is not allowed to edit. Both commands now check the element before the edit
and answer a failed execution result naming the reason (layer hidden, layer
locked, element not editable) while the element stays untouched.

The script places a hotspot on its own layer, then for each of a hidden, a
locked and a hidden+locked layer checks that MoveElements (with and without
copy) and RotateElements report a per-element failure, that the hotspot did
not move, and that no copy appeared. With the layer visible and unlocked again
the same calls succeed and the hotspot really moves.

Not part of the auto-discovered Examples/ (see test_examples.py +
ExpectedOutputs/): it needs to mutate layer attributes and exits non-zero on a
real failure, which would abort that harness's whole run rather than just
failing this one script.
Run manually: python manual_tests/test_move_rotate_elements_not_editable.py
(from the Examples/ folder, with Archicad and the Tapir Add-On running).
"""
import os
import sys

# aclib lives in the parent Examples/ folder - this script is one level below it precisely
# so it stays out of Examples/'s own flat, non-recursive test auto-discovery.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

LAYER_NAME = 'Tapir MoveRotateElements EditTest'

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

def set_layer(hidden, locked):
    result = run('CreateLayers', {
        'layerDataArray': [{'name': LAYER_NAME, 'isHidden': hidden, 'isLocked': locked}],
        'overwriteExisting': True
    })
    return result['attributeIds'][0]['attributeId']

def position_of(elementId):
    details = run('GetDetailsOfElements', {'elements': [{'elementId': elementId}]})['detailsOfElements'][0]
    return details['details']['position']

def hotspot_count():
    return len(run('GetElementsByType', {'elementType': 'Hotspot'})['elements'])

def move(elementId, copy):
    return run('MoveElements', {'elementsWithMoveVectors': [
        {'elementId': elementId, 'moveVector': {'x': 1.0, 'y': 2.0, 'z': 0.0}, 'copy': copy}
    ]})['executionResults'][0]

def rotate(elementId):
    return run('RotateElements', {'elementsWithRotations': [
        {'elementId': elementId, 'rotation': {
            'beginPoint': {'x': 1.0, 'y': 0.0},
            'endPoint': {'x': 0.0, 'y': 1.0},
            'origin': {'x': 10.0, 'y': 10.0}}}
    ]})['executionResults'][0]

# 1) A dedicated, visible and unlocked layer to place the element on.
layerId = set_layer(False, False)
layerIndex = run('GetLayers', {
    'attributeIds': [{'attributeId': layerId}]
})['layers'][0]['index']

# 2) A hotspot is the smallest element that takes an explicit layerIndex and
# whose position GetDetailsOfElements reports.
created = run('CreateHotspots', {
    'hotspotsData': [{'position': {'x': 0.0, 'y': 0.0}, 'layerIndex': layerIndex}]
})
hotspotId = created['elements'][0]['elementId']

# 3) On a hidden, a locked and a hidden+locked layer every edit must be refused
# per element, the hotspot must stay where it is and no copy may appear.
for hidden, locked in ((True, False), (False, True), (True, True)):
    state = 'hidden' if hidden and not locked else 'locked' if locked and not hidden else 'hidden+locked'
    set_layer(hidden, locked)
    before = position_of(hotspotId)
    countBefore = hotspot_count()

    for label, result in (('move', move(hotspotId, False)),
                          ('move with copy', move(hotspotId, True)),
                          ('rotate', rotate(hotspotId))):
        check('{} on a {} layer reports a failed execution result'.format(label, state),
              result['success'] is False, str(result))
        check('{} on a {} layer names the reason'.format(label, state),
              result['success'] is False and ('layer' in result['error']['message'] or 'editable' in result['error']['message']),
              str(result))

    check('the hotspot did not move on a {} layer'.format(state), position_of(hotspotId) == before,
          '{} -> {}'.format(before, position_of(hotspotId)))
    check('no copy appeared on a {} layer'.format(state), hotspot_count() == countBefore,
          '{} -> {}'.format(countBefore, hotspot_count()))

# 4) Visible and unlocked again: the same calls succeed and really change the model.
set_layer(False, False)
before = position_of(hotspotId)
result = move(hotspotId, False)
check('move on an editable layer succeeds', result['success'] is True, str(result))
after = position_of(hotspotId)
check('the hotspot really moved', abs(after['x'] - before['x'] - 1.0) < 1e-6 and abs(after['y'] - before['y'] - 2.0) < 1e-6,
      '{} -> {}'.format(before, after))

countBefore = hotspot_count()
result = move(hotspotId, True)
check('move with copy on an editable layer succeeds', result['success'] is True, str(result))
check('the copy appeared', hotspot_count() == countBefore + 1, '{} -> {}'.format(countBefore, hotspot_count()))

before = position_of(hotspotId)
result = rotate(hotspotId)
check('rotate on an editable layer succeeds', result['success'] is True, str(result))
check('the hotspot really rotated', position_of(hotspotId) != before, '{} -> {}'.format(before, position_of(hotspotId)))

# 5) A guid that is not in the project is reported as such, and does not stop
# the rest of the batch.
result = run('MoveElements', {'elementsWithMoveVectors': [
    {'elementId': {'guid': 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE'}, 'moveVector': {'x': 1.0, 'y': 0.0, 'z': 0.0}},
    {'elementId': hotspotId, 'moveVector': {'x': 1.0, 'y': 0.0, 'z': 0.0}},
]})['executionResults']
check('a guid that is not in the project reports a failed execution result', result[0]['success'] is False, str(result))
check('a missing guid does not block the rest of the batch', result[1]['success'] is True, str(result))

# 6) Clean up: every hotspot that is still on the test layer (the original and the copy).
leftovers = [e for e in run('GetElementsByType', {'elementType': 'Hotspot'})['elements']
             if run('GetDetailsOfElements', {'elements': [e]})['detailsOfElements'][0]['layerIndex'] == layerIndex]
if leftovers:
    run('DeleteElements', {'elements': leftovers})

print('{} passed, {} failed'.format(passes, fails))
sys.exit(0 if fails == 0 else 1)
