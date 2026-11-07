"""
Manual verification for issue #1811: CreateProfiles' moveVertices changes a Profile's geometry
in place, so the skins keep their skinIds and edges, and the Profile keeps its stretch zones and
profile modifiers; edgeOverrides now also take isInnerLine and isCutEndLine.

The script takes the first height-stretchable Profile of the project (TestProject.pla's wall
profiles, or any project with one), copies it to 'ZZTest_MoveVertices' with the vertices of its
top band moved up by 0.1 m, and checks that the copy is 0.1 m taller, still height-stretchable,
has the same skins, edge counts and modifiers, and that the minimum height grew by 0.1 m when
the stretch zone lies below the moved band. It then flips isInnerLine/isCutEndLine of one edge
and reads them back. The copy is deleted at the end.

Run manually from this folder, with Archicad and the Tapir Add-On running:
python test_profile_move_vertices.py
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import aclib

MOVE = 0.1
NAME = 'ZZTest_MoveVertices'

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

def close(a, b):
    return abs(a - b) < 1e-6

def get_profile(attributeId):
    return run('GetProfiles', {'attributeIds': [{'attributeId': attributeId}]})['profiles'][0]

source = None
for attribute in run('GetAttributesByType', {'attributeType': 'Profile'})['attributes']:
    profile = get_profile(attribute['attributeId'])
    if profile.get('heightStretchable') and not attribute['name'].startswith('ZZTest_'):
        source = (attribute['attributeId'], profile)
        break
if source is None:
    raise SystemExit('No height-stretchable Profile in the project.')
sourceId, before = source

# The top band: every vertex above 90% of the profile's height, measured from its lowest vertex.
ys = [c['y'] for s in before['skins'] for c in s['outlineCoords'][1:]]
bottom = min(ys)
topBand = bottom + 0.9 * before['height']

r = run('CreateProfiles', {'overwriteExisting': True, 'profileDataArray': [{
    'name': NAME,
    'sourceAttributeId': {'attributeId': sourceId},
    'moveVertices': [{'region': {'yMin': topBand}, 'vector': {'x': 0.0, 'y': MOVE}}]
}]})
check('CreateProfiles with moveVertices succeeded', 'attributeId' in r['attributeIds'][0], r)
copyId = r['attributeIds'][0]['attributeId']
after = get_profile(copyId)

check('height grew by the moved distance', close(after['height'], before['height'] + MOVE), (before['height'], after['height']))
check('width unchanged', close(after['width'], before['width']), (before['width'], after['width']))
check('still height-stretchable', after.get('heightStretchable') is True, after.get('heightStretchable'))
check('widthStretchable unchanged', after.get('widthStretchable') == before.get('widthStretchable'))
check('minimum height grew by the moved distance (stretch zone kept below the band)',
      close(after['minimumHeight'], before['minimumHeight'] + MOVE), (before['minimumHeight'], after['minimumHeight']))
check('same skins', sorted(s['skinId'] for s in after['skins']) == sorted(s['skinId'] for s in before['skins']))
check('same edge counts', sorted(len(s.get('edges', [])) for s in after['skins']) == sorted(len(s.get('edges', [])) for s in before['skins']))
check('same modifiers', sorted(m.get('name', '') for m in after.get('profileModifiers', [])) ==
      sorted(m.get('name', '') for m in before.get('profileModifiers', [])), after.get('profileModifiers'))

skin = next((s for s in after['skins'] if len(s.get('edges', [])) > 2), None)
if skin is not None:
    edge = skin['edges'][1]
    r = run('CreateProfiles', {'overwriteExisting': True, 'profileDataArray': [{
        'name': NAME,
        'skinOverrides': [{'skinId': skin['skinId'], 'edgeOverrides': [
            {'edgeIndex': 1, 'isInnerLine': not edge['isInnerLine'], 'isCutEndLine': not edge['isCutEndLine']}]}]
    }]})
    check('CreateProfiles with isInnerLine/isCutEndLine succeeded', 'attributeId' in r['attributeIds'][0], r)
    edited = next(s for s in get_profile(copyId)['skins'] if s['skinId'] == skin['skinId'])['edges'][1]
    check('isInnerLine written', edited['isInnerLine'] == (not edge['isInnerLine']), edited)
    check('isCutEndLine written', edited['isCutEndLine'] == (not edge['isCutEndLine']), edited)

run('DeleteAttributes', {'attributesToDelete': [{'attributeType': 'Profile', 'attributeId': {'attributeId': copyId}}]})

print('{} passed, {} failed'.format(passes, fails))
sys.exit(1 if fails else 0)
