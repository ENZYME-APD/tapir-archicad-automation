"""
Live test: open (non-closed) Morph body geometry - previously impossible, CreateMorphs/
ModifyMorphs required at least 4 vertices and 4 faces ("a closed volume") for ANY body,
including bodyType Surface (an open shell, which has no closedness requirement at all).

Covers: a single open face, a box missing one face, wireframe-only loops ("filled": false
on a polygon entry), standalone wire edges (the "wireEdges" field, symmetric between Get
and Create/Modify), and a mix of a filled face + wireframe edges in the same body. Also
checks that Get's reported vertex indices - which Archicad is free to renumber after
ACAPI_Body_Finish - stay internally self-consistent (decoding wireEdges/polygons through
the ALSO-returned "vertices" coordinates, never assuming they match the caller's original
input order).
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-morph-open\archicad-addon\Examples')
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

def decode_edges(body, field):
    """Return each edge as a pair of (x,y,z) tuples, resolved through body['vertices'] -
    never compare raw indices directly, Archicad is free to renumber them after Finish."""
    verts = body.get('vertices', [])
    out = []
    for e in body.get(field, []):
        a, b = e['vertexIds']
        va, vb = verts[a], verts[b]
        out.append(frozenset([(va['x'], va['y'], va['z']), (vb['x'], vb['y'], vb['z'])]))
    return out

print('TEST -- absolute minimum: 2 vertices, 1 standalone wire edge, no face at all')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 6990, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [{'x': 0, 'y': 0, 'z': 0}, {'x': 2, 'y': 0, 'z': 0}],
        'wireEdges': [{'vertexIds': [0, 1]}],
    },
}]})
check('single-edge CreateMorphs succeeds', True, 'elementId' in r['elements'][0])
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('0 polygons', 0, len(body.get('polygons', [])))
check('1 wire edge', 1, len(body.get('wireEdges', [])))
print()

print('TEST -- a lone vertex with no edge at all is rejected (confirmed an Archicad limitation, not Tapir)')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 6980, 'y': 0, 'z': 0},
    'body': {'bodyType': 'Surface', 'vertices': [{'x': 0, 'y': 0, 'z': 0}]},
}]})
check('lone-vertex CreateMorphs fails cleanly (no crash)', True, 'error' in r['elements'][0])
print()

print('TEST -- single open quad face (bodyType Surface, previously rejected: needed >=4 faces)')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 7000, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [{'x': 0, 'y': 0, 'z': 0}, {'x': 2, 'y': 0, 'z': 0}, {'x': 2, 'y': 2, 'z': 0}, {'x': 0, 'y': 2, 'z': 0}],
        'polygons': [{'vertexIds': [0, 1, 2, 3]}],
    },
}]})
check('single-face CreateMorphs succeeds', True, 'elementId' in r['elements'][0])
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('isClosed is False (geometrically open)', False, body.get('isClosed'))
check('1 polygon reported', 1, len(body.get('polygons', [])))
print()

print('TEST -- box missing one face (5 of 6 faces, open shell)')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 7010, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [
            {'x': 0, 'y': 0, 'z': 0}, {'x': 2, 'y': 0, 'z': 0}, {'x': 2, 'y': 2, 'z': 0}, {'x': 0, 'y': 2, 'z': 0},
            {'x': 0, 'y': 0, 'z': 2}, {'x': 2, 'y': 0, 'z': 2}, {'x': 2, 'y': 2, 'z': 2}, {'x': 0, 'y': 2, 'z': 2},
        ],
        'polygons': [
            {'vertexIds': [0, 1, 2, 3]}, {'vertexIds': [4, 5, 1, 0]}, {'vertexIds': [5, 6, 2, 1]},
            {'vertexIds': [6, 7, 3, 2]}, {'vertexIds': [7, 4, 0, 3]},
        ],
    },
}]})
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('isClosed is False (one face missing)', False, body.get('isClosed'))
check('5 polygons reported', 5, len(body.get('polygons', [])))
print()

print('TEST -- wireframe-only rectangle: "filled": false, no face at all')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 7020, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [{'x': 0, 'y': 0, 'z': 0}, {'x': 3, 'y': 0, 'z': 0}, {'x': 3, 'y': 2, 'z': 0}, {'x': 0, 'y': 2, 'z': 0}],
        'polygons': [{'vertexIds': [0, 1, 2, 3], 'filled': False}],
    },
}]})
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('0 filled polygons reported (wireframe only)', 0, len(body.get('polygons', [])))
edges = decode_edges(body, 'wireEdges')
expectedEdges = {
    frozenset([(0, 0, 0), (3, 0, 0)]), frozenset([(3, 0, 0), (3, 2, 0)]),
    frozenset([(3, 2, 0), (0, 2, 0)]), frozenset([(0, 2, 0), (0, 0, 0)]),
}
check('all 4 rectangle sides present as wireEdges (decoded via coordinates)', expectedEdges, set(edges))
print()

print('TEST -- standalone wireEdges field, no polygons at all (round-trip Get -> Create shape)')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 7030, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [{'x': 0, 'y': 0, 'z': 0}, {'x': 3, 'y': 0, 'z': 0}, {'x': 3, 'y': 2, 'z': 0}, {'x': 0, 'y': 2, 'z': 0}],
        'wireEdges': [{'vertexIds': [0, 1]}, {'vertexIds': [1, 2]}, {'vertexIds': [2, 3]}, {'vertexIds': [3, 0]}],
    },
}]})
check('wireEdges-only CreateMorphs succeeds', True, 'elementId' in r['elements'][0])
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('0 polygons', 0, len(body.get('polygons', [])))
edges = decode_edges(body, 'wireEdges')
check('all 4 sides present as wireEdges', expectedEdges, set(edges))
print()

print('TEST -- pyramid: filled square base + 4 wireframe sides to the apex (mixed body)')
r = run('CreateMorphs', {'morphsData': [{
    'basePoint': {'x': 7040, 'y': 0, 'z': 0},
    'body': {
        'bodyType': 'Surface',
        'vertices': [
            {'x': 0, 'y': 0, 'z': 0}, {'x': 3, 'y': 0, 'z': 0}, {'x': 3, 'y': 3, 'z': 0}, {'x': 0, 'y': 3, 'z': 0},
            {'x': 1.5, 'y': 1.5, 'z': 3},
        ],
        'polygons': [{'vertexIds': [0, 1, 2, 3], 'filled': True}],
        'wireEdges': [{'vertexIds': [0, 4]}, {'vertexIds': [1, 4]}, {'vertexIds': [2, 4]}, {'vertexIds': [3, 4]}],
    },
}]})
guid = r['elements'][0]['elementId']['guid']
created.append(guid)
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
body = d.get('body', {})
check('isClosed is False (sides are wireframe, not real faces)', False, body.get('isClosed'))
check('1 filled polygon (the base)', 1, len(body.get('polygons', [])))
apex = (1.5, 1.5, 3.0)
verts = body.get('vertices', [])
apexIdx = next (i for i, v in enumerate (verts) if (v['x'], v['y'], v['z']) == apex)
wireEndpoints = set ()
for e in body.get('wireEdges', []):
    a, b = e['vertexIds']
    other = b if a == apexIdx else (a if b == apexIdx else None)
    if other is not None:
        v = verts[other]
        wireEndpoints.add ((v['x'], v['y'], v['z']))
check('4 wire edges all go from the apex to a distinct base corner', {(0, 0, 0), (3, 0, 0), (3, 3, 0), (0, 3, 0)}, wireEndpoints)
print()

run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created]})

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
