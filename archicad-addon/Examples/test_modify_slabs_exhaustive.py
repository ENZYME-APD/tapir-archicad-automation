"""
Exhaustive live test for ModifySlabs: outline point count changes (grow and shrink), hole
add/remove/clear (including the holes-only shorthand that reuses the current outline), arcs on
both the main outline and individual holes, and combined multi-field modifications in a single
call. Covers the exact reproduction steps from issue #452 (a fatal crash on a same-size outline
resend, and holes-only requests being silently ignored).
"""
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

# =============================================================================
print('TEST -- issue #452 repro: same-outline resend with holes:[] must clear the hole, not crash')
# =============================================================================
outline = [{'x': 0, 'y': 0}, {'x': 10, 'y': 0}, {'x': 10, 'y': 10}, {'x': 0, 'y': 10}]
hole = [{'x': 3, 'y': 3}, {'x': 6, 'y': 3}, {'x': 6, 'y': 6}, {'x': 3, 'y': 6}]
r = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline, 'holes': [{'polygonOutline': hole}]}]})
guid = r['elements'][0]['elementId']['guid']
created_guids.append(guid)
r2 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid}, 'polygonOutline': outline, 'holes': []}]})
check('same-outline + holes:[] succeeds (no crash)', True, r2['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('hole cleared', [], d.get('holes', []))

print('TEST -- holes-only shorthand (no polygonOutline) adds a hole back, then clears it again')
r3 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid}, 'holes': [{'polygonOutline': hole}]}]})
check('holes-only add succeeds', True, r3['executionResults'][0].get('success'))
d2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']
check('hole present, outline untouched', (1, 5), (len(d2.get('holes', [])), len(d2.get('polygonOutline', []))))
r4 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid}, 'holes': []}]})
check('holes-only clear succeeds', True, r4['executionResults'][0].get('success'))
print()

# =============================================================================
print('TEST -- outline point count: increase (4->6) and decrease (6->3)')
# =============================================================================
outline2 = [{'x': 20, 'y': 0}, {'x': 30, 'y': 0}, {'x': 30, 'y': 10}, {'x': 20, 'y': 10}]
r5 = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline2}]})
guid2 = r5['elements'][0]['elementId']['guid']
created_guids.append(guid2)
grown = [{'x': 20, 'y': 0}, {'x': 25, 'y': 0}, {'x': 30, 'y': 0}, {'x': 30, 'y': 5}, {'x': 30, 'y': 10}, {'x': 20, 'y': 10}]
r6 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid2}, 'polygonOutline': grown}]})
check('point count 4->6 succeeds', True, r6['executionResults'][0].get('success'))
d3 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid2}}]})['detailsOfElements'][0]['details']
check('6 outline points present', 7, len(d3.get('polygonOutline', [])))  # +1 for closing duplicate
shrunk = [{'x': 20, 'y': 0}, {'x': 30, 'y': 0}, {'x': 25, 'y': 10}]
r7 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid2}, 'polygonOutline': shrunk}]})
check('point count 6->3 succeeds', True, r7['executionResults'][0].get('success'))
print()

# =============================================================================
print('TEST -- multiple holes: add 3 different shapes (one with its own arc), then shrink to 2')
# =============================================================================
outline3 = [{'x': 40, 'y': 0}, {'x': 80, 'y': 0}, {'x': 80, 'y': 20}, {'x': 40, 'y': 20}]
h1 = {'polygonOutline': [{'x': 42, 'y': 2}, {'x': 46, 'y': 2}, {'x': 46, 'y': 6}, {'x': 42, 'y': 6}]}
h2 = {'polygonOutline': [{'x': 50, 'y': 2}, {'x': 54, 'y': 2}, {'x': 52, 'y': 6}],
      'polygonArcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': 0.4}]}
h3 = {'polygonOutline': [{'x': 58, 'y': 2}, {'x': 62, 'y': 2}, {'x': 64, 'y': 5}, {'x': 62, 'y': 8}, {'x': 58, 'y': 8}]}
r8 = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline3, 'holes': [h1, h2, h3]}]})
guid3 = r8['elements'][0]['elementId']['guid']
created_guids.append(guid3)
d4 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid3}}]})['detailsOfElements'][0]['details']
check('3 holes present after create', 3, len(d4.get('holes', [])))
arced = [h for h in d4.get('holes', []) if h.get('polygonArcs')]
check('1 hole has an arc', 1, len(arced))
r9 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid3}, 'holes': [h2, h3]}]})
check('shrink 3 holes to 2 succeeds', True, r9['executionResults'][0].get('success'))
d5 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid3}}]})['detailsOfElements'][0]['details']
check('2 holes remain', 2, len(d5.get('holes', [])))
print()

# =============================================================================
print('TEST -- arcs: create with 2 outline arcs, replace with 1 different arc, then clear all arcs')
# =============================================================================
outline4 = [{'x': 100, 'y': 0}, {'x': 110, 'y': 0}, {'x': 110, 'y': 10}, {'x': 100, 'y': 10}]
r10 = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline4,
    'polygonArcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': 0.8}, {'begIndex': 2, 'endIndex': 3, 'arcAngle': -0.6}]}]})
guid4 = r10['elements'][0]['elementId']['guid']
created_guids.append(guid4)
d6 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid4}}]})['detailsOfElements'][0]['details']
check('2 arcs present after create', 2, len(d6.get('polygonArcs', [])))
r11 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid4}, 'polygonOutline': outline4,
    'polygonArcs': [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.9}]}]})
check('replace with 1 arc succeeds', True, r11['executionResults'][0].get('success'))
d7 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid4}}]})['detailsOfElements'][0]['details']
check('exactly 1 arc remains (old ones do not stick around)', 1, len(d7.get('polygonArcs', [])))
r12 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid4}, 'polygonOutline': outline4, 'polygonArcs': []}]})
check('clear all arcs succeeds', True, r12['executionResults'][0].get('success'))
d8 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid4}}]})['detailsOfElements'][0]['details']
check('no arcs remain', [], d8.get('polygonArcs', []))
print()

# =============================================================================
print('TEST -- everything at once: grow outline + new arc + replace hole set (different count/style)')
# =============================================================================
outline5 = [{'x': 120, 'y': 0}, {'x': 140, 'y': 0}, {'x': 140, 'y': 20}, {'x': 120, 'y': 20}]
h5a = {'polygonOutline': [{'x': 122, 'y': 2}, {'x': 126, 'y': 2}, {'x': 126, 'y': 6}, {'x': 122, 'y': 6}]}
r13 = run('CreateSlabs', {'slabsData': [{'level': 0, 'thickness': 0.3, 'polygonCoordinates': outline5, 'holes': [h5a]}]})
guid5 = r13['elements'][0]['elementId']['guid']
created_guids.append(guid5)
new_outline5 = [{'x': 120, 'y': 0}, {'x': 130, 'y': 0}, {'x': 140, 'y': 0}, {'x': 140, 'y': 20}, {'x': 120, 'y': 20}]
new_arcs5 = [{'begIndex': 3, 'endIndex': 4, 'arcAngle': 1.1}]
h5b = {'polygonOutline': [{'x': 130, 'y': 14}, {'x': 136, 'y': 14}, {'x': 136, 'y': 18}, {'x': 130, 'y': 18}],
       'polygonArcs': [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.7}]}
h5c = {'polygonOutline': [{'x': 122, 'y': 2}, {'x': 126, 'y': 2}, {'x': 126, 'y': 6}, {'x': 122, 'y': 6}, {'x': 122, 'y': 4}]}
r14 = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': guid5},
    'polygonOutline': new_outline5, 'polygonArcs': new_arcs5, 'holes': [h5b, h5c]}]})
check('combined modify succeeds', True, r14['executionResults'][0].get('success'))
d9 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid5}}]})['detailsOfElements'][0]['details']
check('outline grew to 5 points', 6, len(d9.get('polygonOutline', [])))
check('1 new outline arc', 1, len(d9.get('polygonArcs', [])))
check('2 holes (different set/style than before)', 2, len(d9.get('holes', [])))
print()

cleanup()

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    import sys
    sys.exit(1)
