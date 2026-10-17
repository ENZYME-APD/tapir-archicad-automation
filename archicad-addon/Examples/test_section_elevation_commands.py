"""
Live test for:
- CreateElevations (mirrors CreateSections, header.type = API_ElevationID)
- ChangeWindow fix for issue #466 (databaseId resolution failing because targetDatabase.typeID
  was left unset before ACAPI_Window_GetDatabaseInfo)
- GetSectionsAndElevationsDetails / SetSectionsAndElevationsDetails (exhaustive Section/Elevation
  parameter access) + the optional 'details' field on CreateSections/CreateElevations
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-section-elevation-fix\archicad-addon\Examples')
import aclib

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0
createdGuids = []

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected!r}, got={got!r})')

print('TEST -- CreateElevations creates an element and returns a guid')
r = run('CreateElevations', {'elevationsData': [{
    'startCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 5.0, 'y': 0.0},
    'depth': 2.0,
    'name': 'ZZTest_Elevation',
}]})
print('  raw response:', r)
ok = 'elementId' in r.get('elements', [{}])[0]
check('CreateElevations succeeds', True, ok)
elevGuid = r['elements'][0]['elementId']['guid'] if ok else None
if elevGuid:
    createdGuids.append(elevGuid)
print()

print('TEST -- CreateSections with a "details" block sets exhaustive fields at creation time')
r = run('CreateSections', {'sectionsData': [{
    'startCoordinate': {'x': 10.0, 'y': 0.0},
    'endCoordinate': {'x': 10.0, 'y': 5.0},
    'depth': 1.5,
    'name': 'ZZTest_Section',
    'details': {
        'referenceId': 'ZZ-01',
        'linePenIndex': 3,
        'textPenIndex': 5,
        'showMarkerAtBeginning': True,
        'showMarkerAtEnd': False,
        'continuous': False,
        'markedDistantArea': True,
        'transparent': True,
        'horizontalRange': 'Limited',
        'verticalRange': 'Limited',
        'verticalRangeMin': 0.0,
        'verticalRangeMax': 3.0,
        'storyHandleAppearance': 'All',
        'boundaryDisplayMode': 'NoContours',
        'showOnStories': 'All',
        'markerShowType': 'WingMarkers',
    },
}]})
print('  raw response:', r)
ok = 'elementId' in r.get('elements', [{}])[0]
check('CreateSections with details succeeds', True, ok)
sectionGuid = r['elements'][0]['elementId']['guid'] if ok else None
if sectionGuid:
    createdGuids.append(sectionGuid)
print()

if sectionGuid:
    print('TEST -- GetSectionsAndElevationsDetails reads back exactly what CreateSections.details set')
    d = run('GetSectionsAndElevationsDetails', {'elements': [{'elementId': {'guid': sectionGuid}}]})
    detail = d['detailsOfElements'][0]
    print('  full detail object:', detail)
    check('name round-trips', 'ZZTest_Section', detail.get('name'))
    check('referenceId round-trips', 'ZZ-01', detail.get('referenceId'))
    check('linePenIndex round-trips', 3, detail.get('linePenIndex'))
    check('textPenIndex round-trips', 5, detail.get('textPenIndex'))
    check('showMarkerAtBeginning round-trips', True, detail.get('showMarkerAtBeginning'))
    check('showMarkerAtEnd round-trips', False, detail.get('showMarkerAtEnd'))
    check('continuous round-trips', False, detail.get('continuous'))
    check('markedDistantArea round-trips', True, detail.get('markedDistantArea'))
    check('transparent round-trips', True, detail.get('transparent'))
    check('horizontalRange round-trips', 'Limited', detail.get('horizontalRange'))
    check('verticalRange round-trips', 'Limited', detail.get('verticalRange'))
    check('verticalRangeMin round-trips', 0.0, detail.get('verticalRangeMin'))
    check('verticalRangeMax round-trips', 3.0, detail.get('verticalRangeMax'))
    check('storyHandleAppearance round-trips', 'All', detail.get('storyHandleAppearance'))
    check('boundaryDisplayMode round-trips', 'NoContours', detail.get('boundaryDisplayMode'))
    check('showOnStories round-trips', 'All', detail.get('showOnStories'))
    check('markerShowType round-trips', 'WingMarkers', detail.get('markerShowType'))
    check('read-only isActive is present', True, 'isActive' in detail)
    check('read-only currentPlanConnectivity is present', True, 'currentPlanConnectivity' in detail)
    print()

    print('TEST -- SetSectionsAndElevationsDetails modifies an existing Section')
    setResp = run('SetSectionsAndElevationsDetails', {'elementsWithDetails': [{
        'elementId': {'guid': sectionGuid},
        'details': {
            'linePenIndex': 7,
            'continuous': True,
            'boundaryDisplayMode': 'OverrideContours',
        },
    }]})
    print('  raw response:', setResp)
    check('SetSectionsAndElevationsDetails succeeds', True, setResp['executionResults'][0].get('success'))

    d2 = run('GetSectionsAndElevationsDetails', {'elements': [{'elementId': {'guid': sectionGuid}}]})
    detail2 = d2['detailsOfElements'][0]
    check('linePenIndex updated', 7, detail2.get('linePenIndex'))
    check('continuous updated', True, detail2.get('continuous'))
    check('boundaryDisplayMode updated', 'OverrideContours', detail2.get('boundaryDisplayMode'))
    check('name unaffected by unrelated update', 'ZZTest_Section', detail2.get('name'))
    print()

print('TEST -- ChangeWindow can activate the newly created Elevation via GetDatabaseIdFromNavigatorItemId + databaseId (issue #466 code path)')
tree = run('GetNavigatorItemTree', {'navigatorMapId': 'PublicViewMap'})

def find_elevation_items(node, acc):
    if node.get('type') == 'ElevationItem':
        acc.append(node)
    for wrapper in node.get('children', []):
        find_elevation_items(wrapper['navigatorItem'], acc)
    return acc

elevItems = find_elevation_items(tree['navigatorItemTree'], [])
print(f'  found {len(elevItems)} elevation navigator items')
if elevItems:
    navId = {'guid': elevItems[0]['navigatorItemId']['guid']}
    dbResp = run('GetDatabaseIdFromNavigatorItemId', {'navigatorItemIds': [{'navigatorItemId': navId}]})
    print('  GetDatabaseIdFromNavigatorItemId ->', dbResp)
    dbId = dbResp['databases'][0]['databaseId']
    r3 = run('ChangeWindow', {'windowType': 'Elevation', 'databaseId': dbId})
    print('  ChangeWindow with databaseId ->', r3)
    check('ChangeWindow windowType=Elevation + databaseId succeeds (was APIERR_BADDATABASE before fix)', True, r3.get('success'))
else:
    print('  (skipped databaseId sub-test: no elevation navigator item found in tree)')
print()

if createdGuids:
    run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in createdGuids]})

print('=' * 60)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL')
print('=' * 60)
if fails:
    sys.exit(1)
