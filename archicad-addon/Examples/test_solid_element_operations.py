"""
Test visuel : 5 types d'operations solides, une paire de dalles par operation.

Geometrie adaptee par operation pour rendre le resultat visuellement distinct :

  X=10  Subtraction          : B et A meme niveau/epaisseur, B dedans A (meme Z)   -> trou
  X=16  SubtractionUpwards   : B majoritairement en DESSOUS de A, traverse de bas en haut -> trou
  X=22  SubtractionDownwards : B majoritairement AU-DESSUS de A, traverse de haut en bas -> trou
  X=28  Intersection         : B traverse A, seul le volume commun subsiste -> colonne centrale
  X=34  Addition             : B adjacent a A (meme niveau, cote a cote)           -> fusion en L/rectangle

Lancer avec --cleanup pour supprimer tous les elements crees.
"""

import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples')
import aclib, json

CLEANUP = '--cleanup' in sys.argv

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    print(f'  [{"PASS" if ok else "FAIL"}] {label}')
    if not ok:
        print(f'           expected={expected!r}  got={got!r}')
    if ok: passes += 1
    else:  fails  += 1

# ---------------------------------------------------------------------------
# Mode nettoyage
# ---------------------------------------------------------------------------
if CLEANUP:
    print('==> Nettoyage ...')
    try:
        with open('_solid_test_guids.json') as f:
            guids = json.load(f)
        run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in guids]})
        print(f'  {len(guids)} elements supprimes.')
        import os; os.remove('_solid_test_guids.json')
    except FileNotFoundError:
        print('  Aucun fichier de GUIDs trouve.')
    sys.exit(0)

# ---------------------------------------------------------------------------
# Geometrie des paires — une par operation
# ---------------------------------------------------------------------------
Y0 = 10.0

# Coordonnees helper
def rect(ox, oy, w, h):
    return [{'x': ox,   'y': oy},
            {'x': ox+w, 'y': oy},
            {'x': ox+w, 'y': oy+h},
            {'x': ox,   'y': oy+h}]

def centered(ox, oy, outer_w, outer_h, inner_w, inner_h):
    """Coordonnees d'un rectangle centre dans le rectangle exterieur."""
    mx = ox + (outer_w - inner_w) / 2
    my = oy + (outer_h - inner_h) / 2
    return rect(mx, my, inner_w, inner_h)

CASES = [
    # (operation, description, slab_a_params, slab_b_params)
    # --------------------------------------------------------
    # Subtraction : B exactement au meme niveau/epaisseur qu'A, a l'interieur du plan d'A
    ('Subtraction',
     'B meme Z qu A -> trou central',
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(10, Y0, 4, 4)},
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': centered(10, Y0, 4, 4, 1.5, 1.5)}),
    # --------------------------------------------------------
    # SubtractionUpwards : B clairement EN DESSOUS de A, sans chevauchement volumetrique
    #   A: Z [-0.3 .. 0]  (thin slab au niveau 0)
    #   B: Z [-0.8 .. -0.5] (slab fine, entierement sous A)
    #   SubtractionUpwards projette le plan de B vers le haut a travers A -> trou
    ('SubtractionUpwards',
     'B clairement sous A (pas de contact) -> trou projete vers le haut',
     {'level': 0.0, 'thickness': 0.3, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(16, Y0, 4, 4)},
     {'level': -0.5, 'thickness': 0.3, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': centered(16, Y0, 4, 4, 1.5, 1.5)}),
    # --------------------------------------------------------
    # SubtractionDownwards : B AU-DESSUS d A, traverse de haut en bas
    #   A: Z [-0.5 .. 0]   (top=0, ep=0.5)
    #   B: Z [-1.5 .. 1.0] (top=1.0, ep=2.5) -> majoritairement au-dessus de A
    ('SubtractionDownwards',
     'B majoritairement au-dessus de A -> trou de haut en bas',
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(22, Y0, 4, 4)},
     {'level': 1.0, 'thickness': 2.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': centered(22, Y0, 4, 4, 1.5, 1.5)}),
    # --------------------------------------------------------
    # Intersection : deux dalles identiques (4x4, meme Z) se chevauchant de 2m en X
    #   A: X=28..32  B: X=30..34  -> overlap X=30..32 (2x4m)
    #   Lien applique dans les deux sens -> seule la zone 2x4m commune subsiste
    ('Intersection',
     'A et B se croisent sur 2m -> seule la bande commune subsiste (lien bidirectionnel)',
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(28, Y0, 4, 4)},
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(30, Y0, 4, 4)}),
    # --------------------------------------------------------
    # Addition : B adjacent a A (meme niveau, meme epaisseur, cote a cote)
    #   A: 4x4m a X=34
    #   B: 2x4m a X=38 (partage la face X=38 avec A)
    #   Fusion visible : rectangle 6x4m = un seul solide
    ('Addition',
     'A et B adjacents -> fusion en rectangle 6x4m',
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(34, Y0, 4, 4)},
     {'level': 0.0, 'thickness': 0.5, 'referencePlaneLocation': 'Top',
      'polygonCoordinates': rect(38, Y0, 2, 4)}),
]

OPERATIONS = [c[0] for c in CASES]

# ---------------------------------------------------------------------------
# STEP 1 — Creer toutes les dalles
# ---------------------------------------------------------------------------
print('STEP 1 -- Creation des dalles')

slabs_data = []
for _, _, a_params, b_params in CASES:
    slabs_data.append(a_params)
    slabs_data.append(b_params)

r = run('CreateSlabs', {'slabsData': slabs_data})
created = (r or {}).get('elements', [])
check('dalles creees', len(CASES) * 2, len(created))

if len(created) < len(CASES) * 2:
    print('\nFATAL: creation incomplete.')
    sys.exit(1)

all_guids = [e['elementId']['guid'] for e in created]
pairs = [(created[i*2]['elementId'], created[i*2+1]['elementId']) for i in range(len(CASES))]

with open('_solid_test_guids.json', 'w') as f:
    json.dump(all_guids, f)

for (op, desc, a_p, _), (a, b) in zip(CASES, pairs):
    x = a_p['polygonCoordinates'][0]['x']
    print(f'  X={x:.0f}  {op:25s}  {desc}')

# ---------------------------------------------------------------------------
# STEP 2 — Appliquer les operations solides
# ---------------------------------------------------------------------------
print('\nSTEP 2 -- CreateSolidElementLinks')

solid_links = []
for (op, _, _, _), (a, b) in zip(CASES, pairs):
    solid_links.append({'targetId': a, 'operatorId': b, 'operation': op})
    if op == 'Intersection':
        solid_links.append({'targetId': b, 'operatorId': a, 'operation': op})

create_r = run('CreateSolidElementLinks', {'solidLinks': solid_links})
results = (create_r or {}).get('executionResults', [])
check('liens crees', len(solid_links), len(results))
for res, lk in zip(results, solid_links):
    label = f'  {lk["operation"]}  {lk["targetId"]["guid"][:6]}->{lk["operatorId"]["guid"][:6]}'
    check(label, True, res.get('success', False))

# ---------------------------------------------------------------------------
# STEP 3 — Verifier via GetSolidElementLinks (format groupe par element)
# ---------------------------------------------------------------------------
print('\nSTEP 3 -- GetSolidElementLinks (verification)')

all_elems = [{'elementId': e} for pair in pairs for e in pair]
get_r = run('GetSolidElementLinks', {'elements': all_elems})
entries = (get_r or {}).get('solidLinks', [])
check('une entree par element', len(all_elems), len(entries))

# Construire un index {(target_guid, operator_guid): operation} a partir de la reponse
link_index = {}
for elem_os, entry in zip(all_elems, entries):
    elem_guid = elem_os['elementId']['guid']
    for lk in entry.get('solidLinksWithTheGivenTarget', []):
        op_guid = lk.get('operatorId', {}).get('guid')
        if op_guid:
            link_index[(elem_guid, op_guid)] = lk.get('operation')
    for lk in entry.get('solidLinksWithTheGivenOperator', []):
        tgt_guid = lk.get('targetId', {}).get('guid')
        if tgt_guid:
            link_index[(tgt_guid, elem_guid)] = lk.get('operation')

for lk in solid_links:
    a_guid = lk['targetId']['guid']
    b_guid = lk['operatorId']['guid']
    op     = lk['operation']
    found_op = link_index.get((a_guid, b_guid))
    check(f'  lien {op} {a_guid[:6]}->{b_guid[:6]}', op, found_op)

# ---------------------------------------------------------------------------
# STEP 4 — RemoveSolidElementLinks (teste sur la paire Subtraction)
# ---------------------------------------------------------------------------
print('\nSTEP 4 -- RemoveSolidElementLinks (paire Subtraction)')

a_sub, b_sub = pairs[0]
remove_r = run('RemoveSolidElementLinks', {
    'solidLinks': [{'targetId': a_sub, 'operatorId': b_sub}]
})
rem_results = (remove_r or {}).get('executionResults', [])
check('remove executionResults count', 1, len(rem_results))
if rem_results:
    check('remove success', True, rem_results[0].get('success', False))

# Verifier que le lien a disparu et que les autres sont intacts
get2_r = run('GetSolidElementLinks', {'elements': all_elems})
entries2 = (get2_r or {}).get('solidLinks', [])
link_index2 = {}
for elem_os, entry in zip(all_elems, entries2):
    elem_guid = elem_os['elementId']['guid']
    for lk in entry.get('solidLinksWithTheGivenTarget', []):
        op_guid = lk.get('operatorId', {}).get('guid')
        if op_guid:
            link_index2[(elem_guid, op_guid)] = lk.get('operation')
    for lk in entry.get('solidLinksWithTheGivenOperator', []):
        tgt_guid = lk.get('targetId', {}).get('guid')
        if tgt_guid:
            link_index2[(tgt_guid, elem_guid)] = lk.get('operation')

gone = (a_sub['guid'], b_sub['guid']) not in link_index2
check('lien Subtraction supprime', True, gone)
check('autres liens intacts', len(solid_links) - 1, len(link_index2))

# ---------------------------------------------------------------------------
# Resume
# ---------------------------------------------------------------------------
print(f'\n{passes} PASS | {fails} FAIL')
print()
print('Dalles visibles dans ArchiCAD — verifiez le resultat 3D (vue Y=10..14) :')
for (op, desc, a_p, _) in CASES:
    x = a_p['polygonCoordinates'][0]['x']
    print(f'  X={x:.0f}..{x+4:.0f}  {op:25s}  -> {desc}')
print('  Note: X=10 (Subtraction) : lien supprime en STEP 4, trou disparu.')
print()
print('Nettoyage : python test_solid_element_operations.py --cleanup')

sys.exit(0 if fails == 0 else 1)
