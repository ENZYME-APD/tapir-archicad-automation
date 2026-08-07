"""
Exhaustive live test: Create + GET + SET for the extended Wall/Slab/Column/Beam
type-specific fields (materials, pens, visibility, cover/floor fill, cross section,
profile type / slant, polygon geometry with holes and arcs), plus a regression check
for the ModifySlabs polygon-editing crash (outline/hole changes, point-count growth
and shrinkage, curved outlines).
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-wscb-fix\archicad-addon\Examples')
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

surfaces = run('GetAttributesByType', {'attributeType': 'Surface'})['attributes']
fills = run('GetAttributesByType', {'attributeType': 'Fill'})['attributes']
profiles = run('GetAttributesByType', {'attributeType': 'Profile'})['attributes']
materials = run('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes']
surfaceId = surfaces[0]['attributeId']
surfaceId2 = surfaces[1]['attributeId'] if len(surfaces) > 1 else surfaceId
fillId = fills[0]['attributeId']
profileId = profiles[0]['attributeId']
materialId = materials[0]['attributeId']

# ======================================================================
print('SETUP -- Wall, Slab (curved outline), Column, Beam')
# ======================================================================
rWall = run('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 4000, 'y': 0}, 'endCoordinate': {'x': 4010, 'y': 0},
    'height': 3, 'thickness': 0.25, 'arcAngle': 0.6,
}]})
wallGuid = rWall['elements'][0]['elementId']['guid']
created.append(wallGuid)

rSlab = run('CreateSlabs', {'slabsData': [{
    'level': 0, 'thickness': 0.3,
    'polygonCoordinates': [{'x': 4020, 'y': 0}, {'x': 4030, 'y': 0}, {'x': 4035, 'y': 5}, {'x': 4030, 'y': 10}, {'x': 4020, 'y': 10}],
    'polygonArcs': [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.5}],
}]})
slabGuid = rSlab['elements'][0]['elementId']['guid']
created.append(slabGuid)

rCol = run('CreateColumns', {'columnsData': [{'coordinates': {'x': 4050, 'y': 0, 'z': 0}, 'height': 3, 'depth': 0.3, 'width': 0.3, 'coreAnchor': 'Center'}]})
columnGuid = rCol['elements'][0]['elementId']['guid']
created.append(columnGuid)

rBeam = run('CreateBeams', {'beamsData': [{'begCoordinate': {'x': 4060, 'y': 0}, 'endCoordinate': {'x': 4080, 'y': 0}, 'zCoordinate': 0, 'height': 0.3, 'width': 0.2}]})
beamGuid = rBeam['elements'][0]['elementId']['guid']
created.append(beamGuid)
print()

# ======================================================================
print('TEST -- Wall: geometry + structure + all extended fields combined in one call')
# ======================================================================
# Note: 'height' and 'relativeTopStory' are mutually exclusive (relativeTopStory wins when
# both are set), and Core* referenceLineLocation values need structureType Composite/Profile
# - both are exercised separately below, not combined here.
r = run('ModifyWalls', {'wallsWithDetails': [{
    'elementId': {'guid': wallGuid},
    'begCoordinate': {'x': 4000, 'y': 0}, 'endCoordinate': {'x': 4012, 'y': 0}, 'arcAngle': 0.8,
    'height': 3.2, 'thickness': 0.3, 'offset': 0.05,
    'structureType': 'Basic',
    'referenceLineLocation': 'Outside',
    'zoneRel': 'SubtractFromZone',
    'visibility': {'showOnHome': True, 'showAllAbove': False, 'showAllBelow': False, 'showRelAbove': 0, 'showRelBelow': 0},
    'referenceMaterial': {'overridden': True, 'attributeId': surfaceId},
    'oppositeMaterial': {'overridden': True, 'attributeId': surfaceId2},
    'sideMaterial': {'overridden': True, 'attributeId': surfaceId},
    'cutFillPen': {'overridden': True, 'penIndex': 9},
    'cutFillBackgroundPen': {'overridden': True, 'penIndex': 2},
}]})
check('combined ModifyWalls succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('geometryType stays Straight (curved simple wall)', 'Straight', d.get('geometryType'))
check('arcAngle', 0.8, round(d.get('arcAngle', 0), 6))
check('height', 3.2, d.get('height'))
check('thickness (begThickness)', 0.3, d.get('begThickness'))
check('referenceLineLocation', 'Outside', d.get('referenceLineLocation'))
check('zoneRel', 'SubtractFromZone', d.get('zoneRel'))
check('visibility.showOnHome', True, (d.get('visibility') or {}).get('showOnHome'))
check('isAutoOnStoryVisibility turned off by setting visibility', False, d.get('isAutoOnStoryVisibility'))
check('referenceMaterial.attributeId', surfaceId, (d.get('referenceMaterial') or {}).get('attributeId'))
check('oppositeMaterial.attributeId', surfaceId2, (d.get('oppositeMaterial') or {}).get('attributeId'))
check('cutFillPen.penIndex', 9, (d.get('cutFillPen') or {}).get('penIndex'))
check('cutFillBackgroundPen.penIndex', 2, (d.get('cutFillBackgroundPen') or {}).get('penIndex'))
print()

print('TEST -- Wall: relativeTopStory + topOffset (explicit height must not be set in the same call)')
r = run('ModifyWalls', {'wallsWithDetails': [{'elementId': {'guid': wallGuid}, 'relativeTopStory': 1, 'topOffset': 0.1}]})
check('relativeTopStory + topOffset succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('relativeTopStory', 1, d.get('relativeTopStory'))
check('topOffset (only meaningful with relativeTopStory != 0)', 0.1, d.get('topOffset'))
print()

print('TEST -- Wall: profileType Slanted (single) vs Trapez (double, independent alpha/beta)')
r = run('ModifyWalls', {'wallsWithDetails': [{'elementId': {'guid': wallGuid}, 'profileType': 'Slanted', 'slantAlpha': 1.3, 'slantBeta': 1.4}]})
check('profileType Slanted succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('profileType', 'Slanted', d.get('profileType'))
check('slantAlpha applied', 1.3, d.get('slantAlpha'))
check('slantBeta forced equal to slantAlpha in Slanted mode', 1.3, d.get('slantBeta'))

r = run('ModifyWalls', {'wallsWithDetails': [{'elementId': {'guid': wallGuid}, 'profileType': 'Trapez', 'slantAlpha': 1.2, 'slantBeta': 1.45}]})
check('profileType Trapez succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('profileType', 'Trapez', d.get('profileType'))
check('slantAlpha independent in Trapez mode', 1.2, d.get('slantAlpha'))
check('slantBeta independent in Trapez mode', 1.45, d.get('slantBeta'))

r = run('ModifyWalls', {'wallsWithDetails': [{'elementId': {'guid': wallGuid}, 'profileType': 'Normal'}]})
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('profileType back to Normal', 'Normal', d.get('profileType'))
print()

print('TEST -- Wall: Core reference line location needs Composite structure')
composites = run('GetAttributesByType', {'attributeType': 'Composite'})['attributes']
compositeId = composites[0]['attributeId']
r = run('ModifyWalls', {'wallsWithDetails': [{
    'elementId': {'guid': wallGuid}, 'structureType': 'Composite', 'compositeId': compositeId, 'referenceLineLocation': 'CoreCenter',
}]})
check('structureType+CoreCenter combined succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': wallGuid}}]})['detailsOfElements'][0]['details']
check('structureType is Composite', 'Composite', d.get('structureType'))
check('CoreCenter now takes effect on a Composite wall', 'CoreCenter', d.get('referenceLineLocation'))
print()

# ======================================================================
print('TEST -- Slab: multiple holes (rect + arc) + curved outline growth, combined with materials/fills in one call')
# ======================================================================
d0 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': slabGuid}}]})['detailsOfElements'][0]['details']
check('initial outline is 5 input points + 1 closing duplicate', 6, len(d0.get('polygonOutline', [])))

r = run('ModifySlabs', {'slabsWithDetails': [{
    'elementId': {'guid': slabGuid},
    'polygonOutline': [
        {'x': 4020, 'y': 0}, {'x': 4032, 'y': 0}, {'x': 4038, 'y': 5}, {'x': 4032, 'y': 10}, {'x': 4020, 'y': 10}, {'x': 4015, 'y': 5},
    ],
    'polygonArcs': [{'begIndex': 1, 'endIndex': 2, 'arcAngle': 0.4}, {'begIndex': 4, 'endIndex': 5, 'arcAngle': -0.3}],
    'holes': [
        {'polygonOutline': [{'x': 4022, 'y': 2}, {'x': 4024, 'y': 2}, {'x': 4024, 'y': 4}, {'x': 4022, 'y': 4}]},
        {'polygonOutline': [{'x': 4026, 'y': 6}, {'x': 4028, 'y': 6}, {'x': 4028, 'y': 8}, {'x': 4026, 'y': 8}],
         'polygonArcs': [{'begIndex': 0, 'endIndex': 1, 'arcAngle': 0.5}]},
    ],
    'referencePlaneLocation': 'Top',
    'topMaterial': {'overridden': True, 'attributeId': surfaceId},
    'sideMaterial': {'overridden': True, 'attributeId': surfaceId2},
    'bottomMaterial': {'overridden': True, 'attributeId': surfaceId},
    'cutFillPen': {'overridden': True, 'penIndex': 8},
    'cutFillBackgroundPen': {'overridden': True, 'penIndex': 3},
    'floorFill': {
        'use': True, 'foregroundPen': 5, 'backgroundPen': 2, 'fillId': fillId, 'use3DHatching': True,
        'orientation': {'type': 'Global', 'origin': {'x': 0.0, 'y': 0.0}, 'matrix00': 1.0, 'matrix10': 0.0, 'matrix01': 0.0, 'matrix11': 1.0, 'innerRadius': 0.0},
    },
}]})
check('maximal ModifySlabs (6-pt outline, 2 arcs, 2 holes, materials, fill) succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': slabGuid}}]})['detailsOfElements'][0]['details']
check('polygonOutline has 6 input points + 1 closing duplicate', 7, len(d.get('polygonOutline', [])))
check('polygonArcs has 2 arcs', 2, len(d.get('polygonArcs', [])))
check('holes has 2 holes', 2, len(d.get('holes', [])))
check('referencePlaneLocation', 'Top', d.get('referencePlaneLocation'))
check('topMaterial.attributeId', surfaceId, (d.get('topMaterial') or {}).get('attributeId'))
check('cutFillPen.penIndex', 8, (d.get('cutFillPen') or {}).get('penIndex'))
check('floorFill.use3DHatching', True, (d.get('floorFill') or {}).get('use3DHatching'))
print()

print('TEST -- Slab: shrink outline back down + remove one hole (regression for point-count decrease + hole removal)')
r = run('ModifySlabs', {'slabsWithDetails': [{
    'elementId': {'guid': slabGuid},
    'polygonOutline': [{'x': 4020, 'y': 0}, {'x': 4030, 'y': 0}, {'x': 4030, 'y': 10}, {'x': 4020, 'y': 10}],
    'holes': [
        {'polygonOutline': [{'x': 4022, 'y': 2}, {'x': 4024, 'y': 2}, {'x': 4024, 'y': 4}, {'x': 4022, 'y': 4}]},
    ],
}]})
check('shrink outline + drop 1 hole succeeds (no crash)', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': slabGuid}}]})['detailsOfElements'][0]['details']
check('polygonOutline back to 4 input points + 1 closing duplicate', 5, len(d.get('polygonOutline', [])))
check('holes back to 1 hole', 1, len(d.get('holes', [])))
print()

print('TEST -- Slab: clear all holes with an explicit empty array')
r = run('ModifySlabs', {'slabsWithDetails': [{'elementId': {'guid': slabGuid}, 'holes': []}]})
check('clear holes succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': slabGuid}}]})['detailsOfElements'][0]['details']
check('holes now empty', 0, len(d.get('holes', [])))
print()

# ======================================================================
print('TEST -- Column: everything combined in one ModifyColumns call')
# ======================================================================
# Note: 'height' and 'relativeTopStory' are mutually exclusive, same as Wall - not combined here.
r = run('ModifyColumns', {'columnsWithDetails': [{
    'elementId': {'guid': columnGuid},
    'origin': {'x': 4050, 'y': 1}, 'zCoordinate': 0.2, 'height': 3.5, 'bottomOffset': 0.1,
    'axisRotationAngle': 0.3, 'coreAnchor': 'TopRight',
    'isSlanted': True, 'slantAngle': 0.15, 'slantDirectionAngle': 0.7,
    'wrapping': True,
    'topOffset': 0.2,
    'width': 0.5, 'depth': 0.5,
    'cutFillPen': {'overridden': True, 'penIndex': 6},
    'cutFillBackgroundPen': {'overridden': True, 'penIndex': 1},
    'coverFill': {
        'use': True, 'useFromSurface': False, 'orientationComesFrom3D': True, 'fillId': fillId,
        'foregroundPen': 4, 'backgroundPen': 2, 'transformationType': 'Global',
        'transformation': {'origin': {'x': 0.0, 'y': 0.0}, 'xAxis': {'x': 1.0, 'y': 0.0}, 'yAxis': {'x': 0.0, 'y': 1.0}},
    },
}]})
check('maximal ModifyColumns succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': columnGuid}}]})['detailsOfElements'][0]['details']
check('height', 3.5, d.get('height'))
check('axisRotationAngle', 0.3, d.get('axisRotationAngle'))
check('coreAnchor', 'TopRight', d.get('coreAnchor'))
check('isSlanted', True, d.get('isSlanted'))
check('slantAngle', 0.15, d.get('slantAngle'))
check('slantDirectionAngle', 0.7, d.get('slantDirectionAngle'))
check('wrapping', True, d.get('wrapping'))
check('topOffset', 0.2, d.get('topOffset'))
check('width', 0.5, d.get('width'))
check('depth', 0.5, d.get('depth'))
check('cutFillPen.penIndex', 6, (d.get('cutFillPen') or {}).get('penIndex'))
check('coverFill.orientationComesFrom3D', True, (d.get('coverFill') or {}).get('orientationComesFrom3D'))
print()

print('TEST -- Column: relativeTopStory (separate call, explicit height must not be set alongside)')
r = run('ModifyColumns', {'columnsWithDetails': [{'elementId': {'guid': columnGuid}, 'relativeTopStory': 1}]})
check('relativeTopStory alone succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': columnGuid}}]})['detailsOfElements'][0]['details']
check('relativeTopStory', 1, d.get('relativeTopStory'))
print()

# ======================================================================
print('TEST -- Beam: everything combined in one ModifyBeams call (curved + 2 holes + all fields)')
# ======================================================================
r = run('ModifyBeams', {'beamsWithDetails': [{
    'elementId': {'guid': beamGuid},
    'begCoordinate': {'x': 4060, 'y': 0}, 'endCoordinate': {'x': 4085, 'y': 0},
    'level': 0.1, 'offset': 0.02, 'slantAngle': 0.1, 'arcAngle': 0.4, 'verticalCurveHeight': 0.3,
    'beamShape': 'HorizontallyCurved',
    'isSlanted': True, 'isFlipped': True, 'profileAngle': 0.2,
    'anchorPoint': 'TopRight',
    'width': 0.3, 'height': 0.5,
    'cutFillPen': {'overridden': True, 'penIndex': 10},
    'cutFillBackgroundPen': {'overridden': True, 'penIndex': 4},
    'coverFill': {
        'use': True, 'useFromSurface': False, 'orientationComesFrom3D': False, 'fillId': fillId,
        'foregroundPen': 3, 'backgroundPen': 1, 'transformationType': 'Global',
        'transformation': {'origin': {'x': 0.0, 'y': 0.0}, 'xAxis': {'x': 1.0, 'y': 0.0}, 'yAxis': {'x': 0.0, 'y': 1.0}},
    },
}]})
check('maximal ModifyBeams (no holes) succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': beamGuid}}]})['detailsOfElements'][0]['details']
check('beamShape', 'HorizontallyCurved', d.get('beamShape'))
check('isFlipped', True, d.get('isFlipped'))
check('profileAngle', 0.2, d.get('profileAngle'))
check('anchorPoint', 'TopRight', d.get('anchorPoint'))
check('width', 0.3, d.get('width'))
check('height', 0.5, d.get('height'))
check('cutFillPen.penIndex', 10, (d.get('cutFillPen') or {}).get('penIndex'))
check('coverFill.foregroundPen', 3, (d.get('coverFill') or {}).get('foregroundPen'))
print()

print('TEST -- Beam: 2 holes (rect + circular) replace all holes in a separate call')
r = run('ModifyBeams', {'beamsWithDetails': [{
    'elementId': {'guid': beamGuid},
    'holes': [
        {'type': 'Rectangular', 'showContour': True, 'centerX': 0.3, 'centerZ': 0.1, 'width': 0.1, 'height': 0.08},
        {'type': 'Circular', 'showContour': False, 'centerX': 0.6, 'centerZ': 0.15, 'width': 0.12},
    ],
}]})
check('2-hole ModifyBeams succeeds', True, r['executionResults'][0].get('success'))
d = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': beamGuid}}]})['detailsOfElements'][0]['details']
check('2 holes present', 2, len(d.get('holes', [])))
check('fields from the previous call are still intact', 'HorizontallyCurved', d.get('beamShape'))
print()

# ======================================================================
print('TEST -- Wall: structureType Profile (was previously broken - forced an invalid Poly plan shape)')
rw2 = run('CreateWalls', {'wallsData': [{'begCoordinate': {'x': 4100, 'y': 0}, 'endCoordinate': {'x': 4110, 'y': 0}, 'height': 3, 'thickness': 0.25, 'structureType': 'Profile', 'profileId': profileId}]})
check('CreateWalls with structureType Profile succeeds', True, 'elementId' in rw2['elements'][0])
if 'elementId' in rw2['elements'][0]:
    w2guid = rw2['elements'][0]['elementId']['guid']
    created.append(w2guid)
    dw2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': w2guid}}]})['detailsOfElements'][0]['details']
    check('Wall structureType is Profile', 'Profile', dw2.get('structureType'))
    check('Wall profileId round-trips', profileId, dw2.get('profileId'))
    check('Wall geometryType still Straight (not forced to Polygonal)', 'Straight', dw2.get('geometryType'))
print()

print('TEST -- Column: circleBased, isWidthAndHeightLinked (independent width/depth), buildingMaterialId, profileId')
rc = run('CreateColumns', {'columnsData': [{
    'coordinates': {'x': 4120, 'y': 0, 'z': 0}, 'height': 3,
    'width': 0.3, 'depth': 0.5, 'circleBased': False, 'isWidthAndHeightLinked': False, 'buildingMaterialId': materialId,
}]})
c2guid = rc['elements'][0]['elementId']['guid']
created.append(c2guid)
dc = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': c2guid}}]})['detailsOfElements'][0]['details']
check('Column independent width', 0.3, dc.get('width'))
check('Column independent depth', 0.5, dc.get('depth'))
check('Column circleBased False', False, dc.get('circleBased'))
check('Column buildingMaterialId round-trips', materialId, dc.get('buildingMaterialId'))

r = run('ModifyColumns', {'columnsWithDetails': [{'elementId': {'guid': c2guid}, 'profileId': profileId}]})
check('ModifyColumns to profileId succeeds', True, r['executionResults'][0].get('success'))
dc2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': c2guid}}]})['detailsOfElements'][0]['details']
check('Column profileId round-trips after Modify', profileId, dc2.get('profileId'))
print()

print('TEST -- Beam: isWidthAndHeightLinked (independent width/height), buildingMaterialId, profileId')
rb = run('CreateBeams', {'beamsData': [{
    'begCoordinate': {'x': 4130, 'y': 0}, 'endCoordinate': {'x': 4150, 'y': 0}, 'zCoordinate': 0,
    'width': 0.2, 'height': 0.4, 'isWidthAndHeightLinked': False, 'buildingMaterialId': materialId,
}]})
b2guid = rb['elements'][0]['elementId']['guid']
created.append(b2guid)
db = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': b2guid}}]})['detailsOfElements'][0]['details']
check('Beam independent width', 0.2, db.get('width'))
check('Beam independent height', 0.4, db.get('height'))
check('Beam buildingMaterialId round-trips', materialId, db.get('buildingMaterialId'))

r = run('ModifyBeams', {'beamsWithDetails': [{'elementId': {'guid': b2guid}, 'profileId': profileId}]})
check('ModifyBeams to profileId succeeds', True, r['executionResults'][0].get('success'))
db2 = run('GetDetailsOfElements', {'elements': [{'elementId': {'guid': b2guid}}]})['detailsOfElements'][0]['details']
check('Beam profileId round-trips after Modify', profileId, db2.get('profileId'))
print()

# ======================================================================
run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in created]})

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
