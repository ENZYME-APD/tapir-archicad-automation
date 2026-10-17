"""
Live test for issue #992: ModifySlabs / ModifyColumns with a zCoordinate below the level of the
lowest story must keep the element on the lowest story with a negative offset, not move it to
story 0 with the absolute Z as offset. Also exercises the optional floorIndex of both commands,
which pins the home story the zCoordinate is resolved against (and alone only changes the home
story, as the Create commands already allow).

In a project whose lowest story is index 0 at level 0 (like the test project) the old code gave
the same result by accident; the regression only shows with a basement story below story 0.
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

def details_of(guid):
    return run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': guid}}]})['detailsOfElements'][0]['details']

stories = sorted(run('GetStories')['stories'], key=lambda s: s['index'])
lowest = stories[0]
highest = stories[-1]
belowLowestZ = lowest['level'] - 0.3
print(f'Lowest story: index {lowest["index"]} at level {lowest["level"]}; testing with z = {belowLowestZ}')

# =============================================================================
print('TEST -- ModifyColumns: zCoordinate below the lowest story stays on the lowest story')
# =============================================================================
r = run('CreateColumns', {'columnsData': [{
    'floorIndex': lowest['index'], 'coordinates': {'x': 5000, 'y': 0, 'z': lowest['level'] + 0.25}, 'height': 0.5,
}]})
columnGuid = r['elements'][0]['elementId']['guid']
created_guids.append(columnGuid)

r = run('ModifyColumns', {'columnsWithDetails': [{'elementId': {'guid': columnGuid}, 'zCoordinate': belowLowestZ}]})
check('ModifyColumns succeeds', True, r['executionResults'][0].get('success'))
d = details_of(columnGuid)
check('column floorIndex is the lowest story', lowest['index'], d.get('floorIndex'))
check('column bottomOffset is negative, relative to the lowest story', -0.3, round(d.get('bottomOffset', 0), 6))

print('TEST -- ModifyColumns: floorIndex pins the story zCoordinate is resolved against')
r = run('ModifyColumns', {'columnsWithDetails': [{'elementId': {'guid': columnGuid}, 'floorIndex': highest['index'], 'zCoordinate': belowLowestZ}]})
check('ModifyColumns with floorIndex succeeds', True, r['executionResults'][0].get('success'))
d = details_of(columnGuid)
check('column floorIndex is the pinned story', highest['index'], d.get('floorIndex'))
check('column bottomOffset is relative to the pinned story', round(belowLowestZ - highest['level'], 6), round(d.get('bottomOffset', 0), 6))

print('TEST -- ModifyColumns: floorIndex alone only changes the home story')
r = run('ModifyColumns', {'columnsWithDetails': [{'elementId': {'guid': columnGuid}, 'floorIndex': lowest['index'], 'bottomOffset': 0.1}]})
check('ModifyColumns with floorIndex and bottomOffset succeeds', True, r['executionResults'][0].get('success'))
d = details_of(columnGuid)
check('column floorIndex is back on the lowest story', lowest['index'], d.get('floorIndex'))
check('column bottomOffset is the given one', 0.1, round(d.get('bottomOffset', 0), 6))
print()

# =============================================================================
print('TEST -- ModifySlabs: zCoordinate below the lowest story stays on the lowest story')
# =============================================================================
outline = [{'x': 5010, 'y': 0}, {'x': 5020, 'y': 0}, {'x': 5020, 'y': 10}, {'x': 5010, 'y': 10}]
r = run('CreateSlabs', {'slabsData': [{
    'floorIndex': lowest['index'], 'level': lowest['level'] + 0.25, 'thickness': 0.3, 'polygonCoordinates': outline,
}]})
slabGuid = r['elements'][0]['elementId']['guid']
created_guids.append(slabGuid)

r = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': slabGuid}, 'zCoordinate': belowLowestZ}]})
check('ModifySlabs succeeds', True, r['executionResults'][0].get('success'))
d = details_of(slabGuid)
check('slab floorIndex is the lowest story', lowest['index'], d.get('floorIndex'))
check('slab level is negative, relative to the lowest story', -0.3, round(d.get('level', 0), 6))

print('TEST -- ModifySlabs: floorIndex pins the story zCoordinate is resolved against')
r = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': slabGuid}, 'floorIndex': highest['index'], 'zCoordinate': belowLowestZ}]})
check('ModifySlabs with floorIndex succeeds', True, r['executionResults'][0].get('success'))
d = details_of(slabGuid)
check('slab floorIndex is the pinned story', highest['index'], d.get('floorIndex'))
check('slab level is relative to the pinned story', round(belowLowestZ - highest['level'], 6), round(d.get('level', 0), 6))

print('TEST -- ModifySlabs: floorIndex alone only changes the home story')
r = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': slabGuid}, 'floorIndex': lowest['index']}]})
check('ModifySlabs with floorIndex alone succeeds', True, r['executionResults'][0].get('success'))
d = details_of(slabGuid)
check('slab floorIndex is back on the lowest story', lowest['index'], d.get('floorIndex'))
print()

# =============================================================================
run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created_guids]})

print('=' * 60)
print(f'RESULT :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
