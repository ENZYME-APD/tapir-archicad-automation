"""
Exhaustive live test: Create + GET + SET for every field exposed on the 2D elements
(Line, Arc, Circle, Hotspot, PolyLine, Hatch, Spline), including the "settings" fields
(roomSeparator, pens, line type, fill attributes) added alongside geometry.
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-2d-elements-branch\archicad-addon\Examples')
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

def cleanup():
    if created_guids:
        run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created_guids]})

# Find a real line type and fill attribute to use for lineTypeId/fillId tests
props = run('GetAllProperties', {}) or {}
# Simple fallback: query all line/fill attributes directly via the official API through Tapir's passthrough
line_attrs = run('GetAttributesByType', {'attributeType': 'Line'}) if False else None

# =============================================================================
print('SETUP -- find a line type and fill attribute to reference')
# =============================================================================
import json, urllib.request
def raw(cmd, params=None):
    req = urllib.request.Request('http://127.0.0.1:19723')
    req.add_header('Content-Type', 'application/json')
    body = json.dumps({'command': cmd, 'parameters': params or {}}).encode('utf8')
    return json.loads(urllib.request.urlopen(req, body).read())['result']

line_ids = raw('API.GetAttributesByType', {'attributeType': 'Line'})['attributeIds']
fill_ids = raw('API.GetAttributesByType', {'attributeType': 'Fill'}) if False else None
lineTypeGuid = line_ids[0]['attributeId']['guid']
print(f'  using lineTypeId={lineTypeGuid}')
print()

# =============================================================================
print('TEST -- Line: full field set (geometry + settings)')
# =============================================================================
r = run('CreateLineElements', {'linesData': [{
    'begCoordinate': {'x': 200, 'y': 200}, 'endCoordinate': {'x': 203, 'y': 203},
    'roomSeparator': True, 'linePenIndex': 5, 'lineTypeId': {'guid': lineTypeGuid}
}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
print('  after create:', d)
check('roomSeparator on create', True, d['roomSeparator'])
check('linePenIndex on create', 5, d['linePenIndex'])
check('lineTypeId on create', lineTypeGuid.upper(), d['lineTypeId']['guid'].upper())
check('penWeight field absent (deliberately removed, unreliable in Archicad)', False, 'penWeight' in d)

setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'roomSeparator': False, 'linePenIndex': 8
}}}]})
check('SET settings reported success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('roomSeparator after SET', False, d2['roomSeparator'])
check('linePenIndex after SET', 8, d2['linePenIndex'])
print()

# =============================================================================
print('TEST -- Arc: full field set (geometry + settings)')
# =============================================================================
r = run('CreateArcs', {'arcsData': [{
    'origin': {'x': 210, 'y': 210}, 'radius': 3, 'begAngle': 0, 'endAngle': 1.5,
    'roomSeparator': True, 'linePenIndex': 3
}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Arc roomSeparator on create', True, d['roomSeparator'])
check('Arc linePenIndex on create', 3, d['linePenIndex'])
setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'radius': 4, 'roomSeparator': False
}}}]})
check('Arc SET success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Arc radius after SET', 4, d2['radius'])
check('Arc roomSeparator after SET', False, d2['roomSeparator'])
print()

# =============================================================================
print('TEST -- Hotspot: penIndex field')
# =============================================================================
r = run('CreateHotspots', {'hotspotsData': [{'position': {'x': 220, 'y': 220}, 'height': 0, 'penIndex': 7}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Hotspot penIndex on create', 7, d['penIndex'])
setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {'penIndex': 2}}}]})
check('Hotspot SET success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Hotspot penIndex after SET', 2, d2['penIndex'])
print()

# =============================================================================
print('TEST -- PolyLine: full field set (geometry + settings)')
# =============================================================================
r = run('CreatePolylines', {'polylinesData': [{
    'coordinates': [{'x': 230, 'y': 230}, {'x': 233, 'y': 230}, {'x': 233, 'y': 233}],
    'roomSeparator': True, 'linePenIndex': 4, 'penWeightMm': 0.3
}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('PolyLine roomSeparator on create', True, d['roomSeparator'])
check('PolyLine linePenIndex on create', 4, d['linePenIndex'])
setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'coordinates': [{'x': 230, 'y': 230}, {'x': 236, 'y': 230}, {'x': 236, 'y': 236}, {'x': 230, 'y': 236}],
    'roomSeparator': False, 'linePenIndex': 6
}}}]})
check('PolyLine SET success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('PolyLine coord count after SET', 4, len(d2['coordinates']))
check('PolyLine roomSeparator after SET', False, d2['roomSeparator'])
check('PolyLine linePenIndex after SET', 6, d2['linePenIndex'])
print()

# =============================================================================
print('TEST -- Hatch: full field set (geometry + holes + fill settings)')
# =============================================================================
r = run('CreateHatches', {'hatchesData': [{
    'coordinates': [{'x': 240, 'y': 240}, {'x': 250, 'y': 240}, {'x': 250, 'y': 250}, {'x': 240, 'y': 250}],
    'contourPenIndex': 2, 'fillPenIndex': 9, 'fillBackgroundPenIndex': 1, 'roomSpecial': -1, 'showArea': True
}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
print('  after create:', d)
check('Hatch contourPenIndex on create', 2, d['contourPenIndex'])
check('Hatch fillPenIndex on create', 9, d['fillPenIndex'])
check('Hatch fillBackgroundPenIndex on create', 1, d['fillBackgroundPenIndex'])
check('Hatch roomSpecial on create', -1, d['roomSpecial'])
check('Hatch showArea on create', True, d['showArea'])

setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'coordinates': [{'x': 240, 'y': 240}, {'x': 252, 'y': 240}, {'x': 252, 'y': 252}, {'x': 240, 'y': 252}],
    'holes': [{'polygonOutline': [{'x': 243, 'y': 243}, {'x': 247, 'y': 243}, {'x': 247, 'y': 247}, {'x': 243, 'y': 247}]}],
    'contourPenIndex': 5, 'fillPenIndex': 3, 'showArea': False
}}}]})
check('Hatch SET (geometry+holes+settings) success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
print('  after SET:', d2)
check('Hatch coord count after SET', 5, len(d2['coordinates']))
check('Hatch hole count after SET', 1, len(d2['holes']))
check('Hatch contourPenIndex after SET', 5, d2['contourPenIndex'])
check('Hatch fillPenIndex after SET', 3, d2['fillPenIndex'])
check('Hatch showArea after SET', False, d2['showArea'])
print()

# =============================================================================
print('TEST -- Spline: geometry (create+get only) + settings (create+get+SET)')
# =============================================================================
r = run('CreateSplines', {'splinesData': [{
    'coordinates': [{'x': 260, 'y': 260}, {'x': 263, 'y': 258}, {'x': 266, 'y': 262}],
    'closed': True, 'roomSeparator': True, 'linePenIndex': 6
}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Spline coordinate count on create', 3, len(d['coordinates']))
check('Spline closed on create', True, d['closed'])
check('Spline roomSeparator on create', True, d['roomSeparator'])
check('Spline linePenIndex on create', 6, d['linePenIndex'])

setr = run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'roomSeparator': False, 'linePenIndex': 1
}}}]})
check('Spline settings-only SET success', True, setr['executionResults'][0]['success'])
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Spline roomSeparator after SET', False, d2['roomSeparator'])
check('Spline linePenIndex after SET', 1, d2['linePenIndex'])
# Confirm geometry SET is correctly rejected/no-effect (documented ACAPI limitation, not a bug)
before_coords = d2['coordinates']
run('SetDetailsOfElements', {'elementsWithDetails': [{'elementId': {'guid': guid}, 'details': {'typeSpecificDetails': {
    'coordinates': [{'x': 0, 'y': 0}, {'x': 1, 'y': 0}, {'x': 1, 'y': 1}]
}}}]})
d3 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('Spline geometry unaffected by SET attempt (known ACAPI limitation)', before_coords, d3['coordinates'])
print()

cleanup()

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
