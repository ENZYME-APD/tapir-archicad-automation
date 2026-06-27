import sys
import aclib

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

# =============================================================================
# test_modify_meshes.py
# Creates a test mesh, modifies each ModifyMeshes parameter, reads back
# via GetDetailsOfElements and reports PASS / FAIL / SKIP.
# =============================================================================

PASS  = 'PASS'
FAIL  = 'FAIL'
SKIP  = 'SKIP'
log   = []

def check(name, expected, actual, tol=1e-5):
    if expected is None or actual is None:
        ok = False
    elif isinstance(expected, float):
        ok = abs(expected - actual) < tol
    else:
        ok = expected == actual
    status = PASS if ok else FAIL
    log.append((name, status, expected, actual))
    tag = f'[{status}]'
    print(f'  {tag:<8} {name}')
    if status == FAIL:
        print(f'           expected: {expected!r}')
        print(f'           got     : {actual!r}')
    return ok

def skip(name, reason):
    log.append((name, SKIP, reason, '-'))
    print(f'  [SKIP]   {name} ({reason})')

def get_details(elem_id):
    resp = aclib.RunTapirCommand('GetDetailsOfElements', {
        'elements': [{'elementId': elem_id}]
    }, debug=False)
    if resp is None:
        return None
    items = resp.get('detailsOfElements', [])
    return items[0] if items else None

def modify(elem_id, mesh_data):
    resp = aclib.RunTapirCommand('ModifyMeshes', {
        'meshesData': [{'elementId': elem_id, 'meshData': mesh_data}]
    }, debug=False)
    if resp is None:
        print('    ERROR: no response from ModifyMeshes')
        return False
    r = resp.get('executionResults', [])
    if r and r[0].get('success'):
        return True
    err = r[0].get('error', {}) if r else {}
    print(f'    FAILED: code={err.get("code")}, msg={err.get("message")}')
    return False

def strip_closing(coords):
    """Remove the closing duplicate vertex returned by GetDetailsOfElements."""
    if len(coords) >= 2:
        f, l = coords[0], coords[-1]
        if abs(f['x'] - l['x']) < 1e-5 and abs(f['y'] - l['y']) < 1e-5:
            return coords[:-1]
    return coords

# =============================================================================
# Base mesh: 5x5m rectangle flat at Z=0, on story 0
# =============================================================================
BASE_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 5.0, 'y': 0.0, 'z': 0.0},
    {'x': 5.0, 'y': 5.0, 'z': 0.0},
    {'x': 0.0, 'y': 5.0, 'z': 0.0},
]

print('Creating test mesh...')
cr = aclib.RunTapirCommand('CreateMeshes', {
    'meshesData': [{
        'floorIndex': 0,
        'level': 0.0,
        'skirtLevel': 0.0,
        'skirtType': 'SolidBodyWithSkirt',
        'polygonCoordinates': BASE_POLY,
    }]
}, debug=False)

elem_id = None
if cr:
    elems = cr.get('elements', [])
    if elems and 'elementId' in elems[0]:
        elem_id = elems[0]['elementId']

if elem_id is None:
    print('ERROR: could not create test mesh. Aborting.')
    exit(1)

print(f'  Mesh GUID: {elem_id["guid"][:8]}...')
print()

# =============================================================================
# TEST 1 — level
# =============================================================================
print('TEST 1 — level')
modify(elem_id, {'level': 1.5})
d = get_details(elem_id)
check('level = 1.5', 1.5, d and d.get('details', {}).get('level'))
print()

# =============================================================================
# TEST 2 — skirtType (3 valeurs possibles)
# =============================================================================
print('TEST 2 — skirtType')
for st in ('SurfaceOnlyWithoutSkirt', 'WithSkirt', 'SolidBodyWithSkirt'):
    modify(elem_id, {'skirtType': st})
    d = get_details(elem_id)
    check(f'skirtType = {st}', st, d and d.get('details', {}).get('skirtType'))
print()

# =============================================================================
# TEST 3 — skirtLevel
# =============================================================================
print('TEST 3 — skirtLevel')
modify(elem_id, {'skirtLevel': -2.5})
d = get_details(elem_id)
check('skirtLevel = -2.5', -2.5, d and d.get('details', {}).get('skirtLevel'))
print()

# =============================================================================
# TEST 4 — floorIndex
# =============================================================================
print('TEST 4 — floorIndex')
modify(elem_id, {'floorIndex': 1})
d = get_details(elem_id)
check('floorIndex = 1', 1, d and d.get('floorIndex'))
# Remettre sur le bon etage pour la suite
modify(elem_id, {'floorIndex': 0})
print()

# =============================================================================
# TEST 5 — polygonCoordinates XY (changement de forme)
# =============================================================================
print('TEST 5 — polygonCoordinates XY (changement de forme)')
NEW_POLY = [
    {'x':  1.0, 'y':  1.0, 'z': 0.0},
    {'x': 10.0, 'y':  1.0, 'z': 0.0},
    {'x': 10.0, 'y':  8.0, 'z': 0.0},
    {'x':  1.0, 'y':  8.0, 'z': 0.0},
]
modify(elem_id, {'polygonCoordinates': NEW_POLY})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
check('vertex count = 4', 4, coords and len(coords))
check('vertex[0].x = 1.0', 1.0,  coords and coords[0]['x'])
check('vertex[1].x = 10.0', 10.0, coords and coords[1]['x'])
check('vertex[2].y = 8.0',  8.0,  coords and coords[2]['y'])
print()

# =============================================================================
# TEST 6 — polygonCoordinates Z (terrain par vertices)
# =============================================================================
print('TEST 6 — polygonCoordinates Z (terrain hauteurs variables)')
TERRAIN_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': 0.00},
    {'x': 5.0, 'y': 0.0, 'z': 1.00},
    {'x': 5.0, 'y': 5.0, 'z': 2.00},
    {'x': 0.0, 'y': 5.0, 'z': 0.50},
]
modify(elem_id, {'polygonCoordinates': TERRAIN_POLY})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
if coords and len(coords) >= 4:
    check('Z[0] = 0.00', 0.00, coords[0]['z'])
    check('Z[1] = 1.00', 1.00, coords[1]['z'])
    check('Z[2] = 2.00', 2.00, coords[2]['z'])
    check('Z[3] = 0.50', 0.50, coords[3]['z'])
else:
    check('coords disponibles (>= 4)', True, False)
print()

# =============================================================================
# TEST 7 — nombre de vertices different (3 -> 6 -> 4)
# =============================================================================
print('TEST 7 — changement du nombre de vertices')
# Remettre base 4 sommets
modify(elem_id, {'polygonCoordinates': BASE_POLY})
# Passer a 6 sommets
POLY6 = [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 3.0, 'y': 0.0, 'z': 0.5},
    {'x': 6.0, 'y': 0.0, 'z': 0.0},
    {'x': 6.0, 'y': 6.0, 'z': 1.0},
    {'x': 3.0, 'y': 6.0, 'z': 0.5},
    {'x': 0.0, 'y': 6.0, 'z': 0.0},
]
modify(elem_id, {'polygonCoordinates': POLY6})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
check('6-vertex polygon count', 6, coords and len(coords))
check('Z[1] = 0.5', 0.5, coords and coords[1]['z'])
check('Z[3] = 1.0', 1.0, coords and coords[3]['z'])
# Revenir a 4 sommets
modify(elem_id, {'polygonCoordinates': BASE_POLY})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
check('retour a 4 vertices', 4, coords and len(coords))
print()

# =============================================================================
# TEST 8 — sublines (courbes de niveau interieures)
# =============================================================================
print('TEST 8 — sublines')
SUBLINES = [
    {'coordinates': [
        {'x': 0.5, 'y': 2.5, 'z': 0.8},
        {'x': 2.5, 'y': 2.5, 'z': 1.2},
        {'x': 4.5, 'y': 2.5, 'z': 0.9},
    ]},
    {'coordinates': [
        {'x': 2.5, 'y': 0.5, 'z': 0.5},
        {'x': 2.5, 'y': 4.5, 'z': 1.5},
    ]},
]
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'sublines': SUBLINES})
d = get_details(elem_id)
subs = d and d.get('details', {}).get('sublines', [])
check('sublines count = 2', 2, subs and len(subs))
if subs and len(subs) >= 2:
    c0 = subs[0].get('coordinates', [])
    c1 = subs[1].get('coordinates', [])
    check('subline[0] point count = 3', 3, len(c0))
    check('subline[1] point count = 2', 2, len(c1))
    if len(c0) >= 2:
        check('subline[0][1].z = 1.2', 1.2, c0[1].get('z'))
    if len(c1) >= 2:
        check('subline[1][1].z = 1.5', 1.5, c1[1].get('z'))
print()

# =============================================================================
# TEST 9 — holes (trou dans le maillage)
# =============================================================================
print('TEST 9 — holes')
# Maillage plus grand pour accueillir le trou
OUTER_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 8.0, 'y': 0.0, 'z': 0.0},
    {'x': 8.0, 'y': 8.0, 'z': 0.0},
    {'x': 0.0, 'y': 8.0, 'z': 0.0},
]
HOLE = [
    {'x': 2.0, 'y': 2.0, 'z': 0.0},
    {'x': 6.0, 'y': 2.0, 'z': 0.0},
    {'x': 6.0, 'y': 6.0, 'z': 0.0},
    {'x': 2.0, 'y': 6.0, 'z': 0.0},
]
modify(elem_id, {
    'polygonCoordinates': OUTER_POLY,
    'holes': [{'polygonCoordinates': HOLE}]
})
d = get_details(elem_id)
holes = d and d.get('details', {}).get('holes', [])
check('holes count = 1', 1, holes and len(holes))
if holes:
    hc = strip_closing(holes[0].get('polygonCoordinates', []))
    # ArchiCAD ajoute des vertices de liaison (bridge) entre contour et trou
    # => peut retourner plus de 4 vertices, on verifie juste le minimum
    check('hole vertex count >= 4', True, len(hc) >= 4)
    check('hole vertex[0].x = 2.0', 2.0, hc and hc[0]['x'])

# Supprimer le trou pour la suite
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'holes': []})
print()

# =============================================================================
# TEST 10 — polygonArcs
# =============================================================================
print('TEST 10 — polygonArcs (arc sur arete)')
ARC_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 5.0, 'y': 0.0, 'z': 0.0},
    {'x': 5.0, 'y': 5.0, 'z': 0.0},
    {'x': 0.0, 'y': 5.0, 'z': 0.0},
]
# Arc sur l'arete 1->2 (begIndex=1, endIndex=2, angle=0.5 rad)
ARC = [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.5}]
modify(elem_id, {'polygonCoordinates': ARC_POLY, 'polygonArcs': ARC})
d = get_details(elem_id)
arcs = d and d.get('details', {}).get('polygonArcs', [])
check('polygonArcs count = 1', 1, arcs and len(arcs))
if arcs:
    check('arc begIndex = 1', 1, arcs[0].get('begIndex'))
    check('arc endIndex = 2', 2, arcs[0].get('endIndex'))
    check('arc angle = 0.5', 0.5, arcs[0].get('arcAngle'))
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'polygonArcs': []})
print()

# =============================================================================
# TEST 11 — parametres lus en retour via GetDetailsOfElements
# =============================================================================
print('TEST 11 — ridges / showLines / contourPen / levelPen / lineTypeIndex')
for value, data, field, expected in [
    ('ridges=AllSharp',    {'ridges': 'AllSharp'},    'ridges',        'AllSharp'),
    ('ridges=AllSmooth',   {'ridges': 'AllSmooth'},   'ridges',        'AllSmooth'),
    ('ridges=UserDefined', {'ridges': 'UserDefined'},  'ridges',        'UserDefined'),
    ('showLines=True',     {'showLines': True},        'showLines',     True),
    ('showLines=False',    {'showLines': False},       'showLines',     False),
    ('contourPen=3',       {'contourPen': 3},          'contourPen',    3),
    ('levelPen=5',         {'levelPen': 5},            'levelPen',      5),
    ('lineTypeIndex=2',    {'lineTypeIndex': 2},       'lineTypeIndex', 2),
]:
    modify(elem_id, data)
    d = get_details(elem_id)
    got = d and d.get('details', {}).get(field)
    check(value, expected, got)
print()

# =============================================================================
# TEST 12 — suppression des sublines (sublines: [])
# =============================================================================
print('TEST 12 — suppression des sublines')
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'sublines': SUBLINES})
d = get_details(elem_id)
subs = d and d.get('details', {}).get('sublines', [])
check('sublines presentes avant suppression', 2, subs and len(subs))
modify(elem_id, {'sublines': []})
d = get_details(elem_id)
subs_after = d and d.get('details', {}).get('sublines')
check('suppression sans changer polygone', 0, len(subs_after) if subs_after is not None else 0)
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'sublines': SUBLINES})
modify(elem_id, {'polygonCoordinates': BASE_POLY, 'sublines': []})
d = get_details(elem_id)
subs_after2 = d and d.get('details', {}).get('sublines')
check('suppression avec changement polygone', 0, len(subs_after2) if subs_after2 is not None else 0)
print()

# =============================================================================
# TEST 13 — Z negatifs dans polygonCoordinates
# =============================================================================
print('TEST 13 — Z negatifs')
NEG_Z_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': -1.0},
    {'x': 5.0, 'y': 0.0, 'z':  0.0},
    {'x': 5.0, 'y': 5.0, 'z': -0.5},
    {'x': 0.0, 'y': 5.0, 'z': -2.0},
]
modify(elem_id, {'polygonCoordinates': NEG_Z_POLY})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
if coords and len(coords) == 4:
    check('Z[0] = -1.0', -1.0, coords[0]['z'])
    check('Z[2] = -0.5', -0.5, coords[2]['z'])
    check('Z[3] = -2.0', -2.0, coords[3]['z'])
else:
    check('coords disponibles (Z negatifs)', True, False)
modify(elem_id, {'polygonCoordinates': BASE_POLY})
print()

# =============================================================================
# TEST 14 — polygone a 3 vertices (minimum legal)
# =============================================================================
print('TEST 14 — polygone a 3 vertices (minimum)')
TRI_POLY = [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 6.0, 'y': 0.0, 'z': 1.0},
    {'x': 3.0, 'y': 5.0, 'z': 2.0},
]
modify(elem_id, {'polygonCoordinates': TRI_POLY})
d = get_details(elem_id)
coords = d and strip_closing(d.get('details', {}).get('polygonCoordinates', []))
# ArchiCAD peut ajouter des vertices intermediaires au triangle
check('triangle count >= 3', True, coords and len(coords) >= 3)
check('triangle Z[2] = 2.0', 2.0, coords and coords[2]['z'])
modify(elem_id, {'polygonCoordinates': BASE_POLY})
print()

# =============================================================================
# TEST 15 — modification simultanée de plusieurs parametres
# =============================================================================
print('TEST 15 — modification simultanee (level + skirtType + polygone + sublines)')
modify(elem_id, {
    'level':      2.0,
    'skirtType':  'WithSkirt',
    'skirtLevel': -1.0,
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 4.0, 'y': 0.0, 'z': 0.5},
        {'x': 4.0, 'y': 4.0, 'z': 1.0},
        {'x': 0.0, 'y': 4.0, 'z': 0.5},
    ],
    'sublines': [{'coordinates': [
        {'x': 2.0, 'y': 0.5, 'z': 0.3},
        {'x': 2.0, 'y': 3.5, 'z': 0.7},
    ]}],
})
d = get_details(elem_id)
det = d and d.get('details', {})
check('multi: level = 2.0',       2.0,         det and det.get('level'))
check('multi: skirtType',         'WithSkirt',  det and det.get('skirtType'))
check('multi: skirtLevel = -1.0', -1.0,         det and det.get('skirtLevel'))
coords = det and strip_closing(det.get('polygonCoordinates', []))
check('multi: Z[1] = 0.5', 0.5, coords and coords[1]['z'])
subs = det and det.get('sublines', [])
check('multi: sublines count = 1', 1, subs and len(subs))
modify(elem_id, {'level': 0.0, 'skirtLevel': 0.0, 'polygonCoordinates': BASE_POLY, 'sublines': []})
print()

# =============================================================================
# TEST 16 — plusieurs meshes en une seule commande
# =============================================================================
print('TEST 16 — plusieurs meshes en une seule commande')
cr2 = aclib.RunTapirCommand('CreateMeshes', {
    'meshesData': [{
        'floorIndex': 0,
        'level': 0.0,
        'polygonCoordinates': [
            {'x': 10.0, 'y': 0.0, 'z': 0.0},
            {'x': 15.0, 'y': 0.0, 'z': 0.0},
            {'x': 15.0, 'y': 5.0, 'z': 0.0},
            {'x': 10.0, 'y': 5.0, 'z': 0.0},
        ],
    }]
}, debug=False)
elem_id2 = None
if cr2:
    elems2 = cr2.get('elements', [])
    if elems2 and 'elementId' in elems2[0]:
        elem_id2 = elems2[0]['elementId']

if elem_id2:
    resp = aclib.RunTapirCommand('ModifyMeshes', {
        'meshesData': [
            {'elementId': elem_id,  'meshData': {'level': 0.5}},
            {'elementId': elem_id2, 'meshData': {'level': 1.5}},
        ]
    }, debug=False)
    results2 = resp and resp.get('executionResults', [])
    check('multi-mesh: 2 resultats', 2, results2 and len(results2))
    check('multi-mesh: mesh1 success', True, results2 and results2[0].get('success'))
    check('multi-mesh: mesh2 success', True, results2 and results2[1].get('success'))
    d1 = get_details(elem_id)
    d2 = get_details(elem_id2)
    check('multi-mesh: mesh1 level = 0.5', 0.5, d1 and d1.get('details', {}).get('level'))
    check('multi-mesh: mesh2 level = 1.5', 1.5, d2 and d2.get('details', {}).get('level'))
else:
    check('creation mesh2', True, False)
print()

# =============================================================================
# TEST 17 — cas d'erreur : polygone < 3 vertices
# =============================================================================
print('TEST 17 — erreur : polygone < 3 vertices')
ok = modify(elem_id, {'polygonCoordinates': [
    {'x': 0.0, 'y': 0.0, 'z': 0.0},
    {'x': 1.0, 'y': 0.0, 'z': 0.0},
]})
check('erreur attendue (< 3 vertices)', False, ok)
print()

# =============================================================================
# TEST 18 — cas d'erreur : elementId inexistant
# =============================================================================
print('TEST 18 — erreur : elementId inexistant')
fake_id = {'guid': '00000000-0000-0000-0000-000000000000', 'type': 'Mesh'}
resp = aclib.RunTapirCommand('ModifyMeshes', {
    'meshesData': [{'elementId': fake_id, 'meshData': {'level': 1.0}}]
}, debug=False)
r = resp and resp.get('executionResults', [])
success = r and r[0].get('success', True)
check('erreur attendue (guid inexistant)', False, success)
print()

# =============================================================================
# TEST 19 — cas d'erreur : element non-mesh (on cree un mur)
# =============================================================================
print('TEST 19 — erreur : element non-mesh')
cr_wall = aclib.RunTapirCommand('CreateWalls', {
    'wallsData': [{
        'begCoordinate': {'x': 20.0, 'y': 0.0},
        'endCoordinate': {'x': 25.0, 'y': 0.0},
        'zCoordinate': 0.0,
        'height': 3.0,
        'thickness': 0.2,
    }]
}, debug=False)
wall_id = None
if cr_wall:
    we = cr_wall.get('elements', [])
    if we and 'elementId' in we[0]:
        wall_id = we[0]['elementId']

if wall_id:
    resp = aclib.RunTapirCommand('ModifyMeshes', {
        'meshesData': [{'elementId': wall_id, 'meshData': {'level': 1.0}}]
    }, debug=False)
    r = resp and resp.get('executionResults', [])
    success = r and r[0].get('success', True)
    check('erreur attendue (mur != mesh)', False, success)
    aclib.RunTapirCommand('DeleteElements', {'elements': [{'elementId': wall_id}]}, debug=False)
else:
    check('creation mur pour test', True, False)
print()

# =============================================================================
# Nettoyage
# =============================================================================
print('Suppression des meshes de test...')
to_delete = [{'elementId': elem_id}]
if elem_id2:
    to_delete.append({'elementId': elem_id2})
aclib.RunTapirCommand('DeleteElements', {'elements': to_delete}, debug=False)

# =============================================================================
# Bilan
# =============================================================================
passed  = sum(1 for _, s, _, _ in log if s == PASS)
failed  = sum(1 for _, s, _, _ in log if s == FAIL)
skipped = sum(1 for _, s, _, _ in log if s == SKIP)

print()
print('=' * 60)
print(f'BILAN :  {passed} PASS  |  {failed} FAIL  |  {skipped} SKIP')
print('=' * 60)
if failed:
    print()
    print('Tests en echec :')
    for name, status, exp, got in log:
        if status == FAIL:
            print(f'  FAIL  {name}')
            print(f'        attendu : {exp!r}')
            print(f'        obtenu  : {got!r}')
