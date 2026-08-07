"""
Live test for issue #513: CreateLayerCombinations/ModifyLayerCombinations (via
overwriteExisting) silently ignored per-layer isHidden/isLocked/isWireframe/
intersectionGroupNr - the created combination's name and layer membership were saved, but
every per-layer stat came back as the default (visible, unlocked, not wireframe, group 1).

Root cause: a layer combination's stat table is not sparse - Archicad expects one
API_LayerStat entry per PROJECT layer (lNumb is documented as "the same value in all layer
combinations", i.e. the project's total layer count), not just for the layers mentioned in
the request. Supplying only a partial table made Archicad read past it into uninitialized
memory for every layer not explicitly listed - confirmed live: those layers came back with
scattered garbage flags (mostly isHidden=true, occasionally isWireframe=true), not the
documented "new layers appear as visible and unlocked" default.

Fix seeds a full stat table (every project layer, defaulted to visible/unlocked/group 1, or
preserved from the existing combination when modifying) before applying the caller's
overrides on top. A second, independent bug was found and fixed while testing this: the
override step used GS::HashTable::Add, which is a no-op returning false when the key already
exists (every key was already seeded) - Put is required to actually overwrite.
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-layercomb-fix\archicad-addon\Examples')
import aclib

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0
created = []

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected!r}, got={got!r})')

def get_combination(guid):
    return run('GetLayerCombinations', {'attributes': [{'attributeId': {'guid': guid}}]})['layerCombinations'][0]['layerCombination']

def default_stat(layer_guid):
    return {'attributeId': {'guid': layer_guid}, 'isHidden': False, 'isLocked': False, 'isWireframe': False, 'intersectionGroupNr': 1}

layers = run('GetAttributesByType', {'attributeType': 'Layer'})['attributes']
totalLayers = len(layers)
print(f'SETUP -- project has {totalLayers} layers')
print()

print('TEST -- single targeted layer: flags apply to it, every other layer stays default')
target = layers[5]['attributeId']
r = run('CreateLayerCombinations', {'layerCombinationDataArray': [{
    'name': 'TEST_single_layer',
    'layers': [{'attributeId': target, 'isHidden': True, 'isLocked': True, 'isWireframe': True, 'intersectionGroupNr': 7}],
}], 'overwriteExisting': False})
check('CreateLayerCombinations succeeds', True, 'attributeId' in r['attributeIds'][0])
guid = r['attributeIds'][0]['attributeId']['guid']
created.append(guid)
d = get_combination(guid)
check('all project layers present in the combination', totalLayers, len(d['layers']))
targeted = next(l for l in d['layers'] if l['attributeId']['guid'] == target['guid'])
check('targeted layer got the requested flags', {'attributeId': target, 'isHidden': True, 'isLocked': True, 'isWireframe': True, 'intersectionGroupNr': 7}, targeted)
untouched = [l for l in d['layers'] if l['attributeId']['guid'] != target['guid']]
check('every other layer is still exactly default (no garbage)', True, all(l == default_stat(l['attributeId']['guid']) for l in untouched))
print()

print('TEST -- multiple layers targeted in one request, each gets only its own flags')
r = run('CreateLayerCombinations', {'layerCombinationDataArray': [{
    'name': 'TEST_multi_layer',
    'layers': [
        {'attributeId': layers[1]['attributeId'], 'isHidden': True},
        {'attributeId': layers[2]['attributeId'], 'isLocked': True},
        {'attributeId': layers[3]['attributeId'], 'isWireframe': True, 'intersectionGroupNr': 5},
    ],
}], 'overwriteExisting': False})
guid2 = r['attributeIds'][0]['attributeId']['guid']
created.append(guid2)
d2 = get_combination(guid2)
byGuid = {l['attributeId']['guid']: l for l in d2['layers']}
check('layer[1] isHidden only', {'attributeId': layers[1]['attributeId'], 'isHidden': True, 'isLocked': False, 'isWireframe': False, 'intersectionGroupNr': 1}, byGuid[layers[1]['attributeId']['guid']])
check('layer[2] isLocked only', {'attributeId': layers[2]['attributeId'], 'isHidden': False, 'isLocked': True, 'isWireframe': False, 'intersectionGroupNr': 1}, byGuid[layers[2]['attributeId']['guid']])
check('layer[3] isWireframe + group 5', {'attributeId': layers[3]['attributeId'], 'isHidden': False, 'isLocked': False, 'isWireframe': True, 'intersectionGroupNr': 5}, byGuid[layers[3]['attributeId']['guid']])
print()

print('TEST -- full layer list with one flag flipped (matches the issue\'s own falsification test)')
fullPayload = [{'attributeId': l['attributeId'], **({'isLocked': True} if i == 20 else {})} for i, l in enumerate(layers)]
r = run('CreateLayerCombinations', {'layerCombinationDataArray': [{'name': 'TEST_full_list', 'layers': fullPayload}], 'overwriteExisting': False})
guid3 = r['attributeIds'][0]['attributeId']['guid']
created.append(guid3)
d3 = get_combination(guid3)
target3 = next(l for l in d3['layers'] if l['attributeId']['guid'] == layers[20]['attributeId']['guid'])
check('the one flipped layer is isLocked', {'attributeId': layers[20]['attributeId'], 'isHidden': False, 'isLocked': True, 'isWireframe': False, 'intersectionGroupNr': 1}, target3)
others3 = [l for l in d3['layers'] if l['attributeId']['guid'] != layers[20]['attributeId']['guid']]
check('every other layer is still exactly default', True, all(l == default_stat(l['attributeId']['guid']) for l in others3))
print()

print('TEST -- modify (overwriteExisting): touching one layer preserves another layer\'s existing custom flags')
r = run('CreateLayerCombinations', {'layerCombinationDataArray': [{
    'attributeId': {'guid': guid},
    'name': 'TEST_single_layer',
    'layers': [{'attributeId': layers[10]['attributeId'], 'isLocked': True, 'intersectionGroupNr': 3}],
}], 'overwriteExisting': True})
check('ModifyLayerCombinations (overwriteExisting) succeeds', True, 'attributeId' in r['attributeIds'][0])
d4 = get_combination(guid)
byGuid4 = {l['attributeId']['guid']: l for l in d4['layers']}
check('originally-targeted layer keeps its flags from before the modify', {'attributeId': target, 'isHidden': True, 'isLocked': True, 'isWireframe': True, 'intersectionGroupNr': 7}, byGuid4[target['guid']])
check('newly-targeted layer got its new flags', {'attributeId': layers[10]['attributeId'], 'isHidden': False, 'isLocked': True, 'isWireframe': False, 'intersectionGroupNr': 3}, byGuid4[layers[10]['attributeId']['guid']])
print()

run('DeleteAttributes', {'attributesToDelete': [{'attributeType': 'LayerCombination', 'attributeId': {'attributeId': {'guid': g}}} for g in created]})

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
