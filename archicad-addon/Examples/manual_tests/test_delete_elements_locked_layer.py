"""
Manual verification for issue #666: DeleteElements used to answer a blanket
{"success": true} even when ACAPI_Element_Delete silently skipped an element
it was not allowed to delete (its layer was locked), so a caller could not
tell "deleted" from "silently ignored". The command now answers one execution
result per input element, decided by whether the element is really gone.

The script places a hotspot on its own layer, locks the layer, and checks that
DeleteElements reports a per-element failure while the element survives; then
it unlocks the layer and checks that the same call succeeds and the element is
gone. A guid that never existed must get its own failed result too.

Not part of the auto-discovered Examples/ (see test_examples.py +
ExpectedOutputs/): it needs to mutate layer attributes and exits non-zero on a
real failure, which would abort that harness's whole run rather than just
failing this one script.
Run manually: python manual_tests/test_delete_elements_locked_layer.py (from
the Examples/ folder, with Archicad and the Tapir Add-On running).
"""
import os
import sys

# aclib lives in the parent Examples/ folder - this script is one level below it precisely
# so it stays out of Examples/'s own flat, non-recursive test auto-discovery.
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

LAYER_NAME = 'Tapir DeleteElements LockTest'

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

def set_layer(locked):
    result = run('CreateLayers', {
        'layerDataArray': [{'name': LAYER_NAME, 'isLocked': locked}],
        'overwriteExisting': True
    })
    return result['attributeIds'][0]['attributeId']

# 1) A dedicated, initially unlocked layer to place the element on.
layerId = set_layer(False)
layerIndex = run('GetLayers', {
    'attributeIds': [{'attributeId': layerId}]
})['layers'][0]['index']

# 2) A hotspot is the smallest element that takes an explicit layerIndex.
created = run('CreateHotspots', {
    'hotspotsData': [{'position': {'x': 0.0, 'y': 0.0}, 'layerIndex': layerIndex}]
})
hotspotId = created['elements'][0]['elementId']

# 3) Locked layer: the delete must be refused per element...
set_layer(True)
result = run('DeleteElements', {'elements': [{'elementId': hotspotId}]})
check('delete on a locked layer reports a failed execution result',
      result['executionResults'][0]['success'] is False, str(result))

# ...and the element must still be in the project.
details = run('GetDetailsOfElements', {
    'elements': [{'elementId': hotspotId}]
})['detailsOfElements'][0]
check('the element survives the refused delete', 'error' not in details, str(details))

# 4) Unlocked layer: the same call succeeds and the element is gone.
set_layer(False)
result = run('DeleteElements', {'elements': [{'elementId': hotspotId}]})
check('delete on an unlocked layer succeeds',
      result['executionResults'][0]['success'] is True, str(result))

details = run('GetDetailsOfElements', {
    'elements': [{'elementId': hotspotId}]
})['detailsOfElements'][0]
check('the element is gone after the successful delete', 'error' in details, str(details))

# 5) A guid that never existed gets its own failed result.
result = run('DeleteElements', {
    'elements': [{'elementId': {'guid': 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE'}}]
})
check('a nonexistent element reports a failed execution result',
      result['executionResults'][0]['success'] is False, str(result))

print('{} passed, {} failed'.format(passes, fails))
sys.exit(0 if fails == 0 else 1)
