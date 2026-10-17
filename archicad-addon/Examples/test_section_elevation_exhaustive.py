"""
Exhaustive round-trip test for every field of SectionElevationSettableDetails
(GetSectionsAndElevationsDetails / SetSectionsAndElevationsDetails / the optional
'details' block on CreateSections and CreateElevations).

For each of the ~65 settable fields: set a non-default value, read it back, and check it
matches exactly. Run once through CreateSections(details=...) and once through
SetSectionsAndElevationsDetails on an existing element, so both write paths are covered.
"""
import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-section-elevation-fix\archicad-addon\Examples')
import aclib

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

passes = fails = 0

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    tag = 'PASS' if ok else 'FAIL'
    if ok:
        passes += 1
    else:
        fails += 1
    print(f'  [{tag}] {label}  (expected={expected!r}, got={got!r})')

lineTypes = run('GetAttributesByType', {'attributeType': 'Line'})['attributes']
fills = run('GetAttributesByType', {'attributeType': 'Fill'})['attributes']
lineTypeIdA = lineTypes[0]['attributeId']
lineTypeIdB = lineTypes[1]['attributeId']
lineTypeIdC = lineTypes[2]['attributeId']
lineTypeIdD = lineTypes[3]['attributeId']
fillIdA = fills[0]['attributeId']
fillIdB = fills[1]['attributeId']

# field name -> (value to set, expected value on readback; defaults to same as set value)
ALL_FIELDS = {
    'name':                                          'ZZExhaustive',
    'referenceId':                                   'EXH-42',
    'linePenIndex':                                  11,
    'textPenIndex':                                  12,
    'lineTypeId':                                    lineTypeIdA,
    'sectionLinePenIndex':                            13,
    'sectionFillPenIndex':                            14,
    'sectionFillBackgroundPenIndex':                  15,
    'horizontalRange':                               'Limited',
    'verticalRange':                                 'Limited',
    'verticalRangeStoryBaseNumber':                   0,
    'relativeToStory':                               True,
    'verticalRangeMin':                              1.25,
    'verticalRangeMax':                              4.75,
    'markerSize':                                    3.3,
    'showMarkerAtBeginning':                         True,
    'showMarkerAtEnd':                               True,
    'useElementPens':                                False,
    'showLineAtBeginning':                           True,
    'showLineAtMiddle':                              True,
    'showLineAtEnd':                                 True,
    'textSize':                                      4.2,
    'fontIndex':                                     1,
    'effectVectorHatch':                             True,
    'effectVectorShadow':                            True,
    'effectSunFrom3D':                               True,
    'shadowFillId':                                  fillIdA,
    'shadowFillPenIndex':                             16,
    'shadowFillBackgroundPenIndex':                   17,
    'limitLineTypeId':                               lineTypeIdB,
    'limitLinePenIndex':                              18,
    'continuous':                                     False,
    'sunAzimuth':                                     123.5,
    'sunAltitude':                                    45.5,
    'useOneModelPen':                                True,
    'modelPenIndex':                                  19,
    'modelUncutSurfaceFillType':                      'MaterialColorShaded',
    'modelUncutSurfaceFillBackgroundPenIndex':        20,
    'useUncutSurfaceFill':                           True,
    'transparent':                                    True,
    'markedDistantArea':                             True,
    'useOneDistantAreaPen':                          True,
    'distantAreaPenIndex':                            21,
    # NOTE: distantAreaShadowFillId/PenIndex/BackgroundPenIndex/distantAreaUncutSurfaceFillType are
    # deliberately excluded here - confirmed live (see git history / commit message) that Archicad
    # always mirrors them from shadowFillId/shadowFillPenIndex/shadowFillBackgroundPenIndex/
    # modelUncutSurfaceFillType regardless of what's written. They are read-only in the schema now;
    # checked separately below via READ_ONLY_MIRRORED_FIELDS.
    'distantAreaUncutSurfaceFillBackgroundPenIndex':  24,
    'useUncutSurfaceFillInDistantArea':              True,
    'storyHandleAppearance':                          'DisplayOnly',
    'storyHandleLineTypeId':                          lineTypeIdC,
    'storyHandleLinePenIndex':                        25,
    'storyHandleMarkerPenIndex':                      26,
    'storyHandleUseSymbolPens':                       False,
    'storyHandleLeftMarkerVisible':                   False,
    'storyHandleRightMarkerVisible':                  True,
    'storyHandleLineVisible':                         False,
    'storyHandleLeftMarkerOffset':                    0.5,
    'storyHandleRightMarkerOffset':                   0.75,
    'storyHandleMarkerFontIndex':                     1,
    'storyHandleMarkerTextSize':                      2.1,
    'storyHandleMarkerSize':                          1.9,
    'boundaryDisplayMode':                            'OverrideContours',
    'boundaryPenIndex':                                27,
    'boundaryLineTypeId':                             lineTypeIdD,
    'showOnStories':                                  'OneStory',
    'storyToShowOn':                                  0,
    'markerShowType':                                 'WingMarkers',
    'intendedPlanConnectivity':                       'Offline',
}

def expected_value(field, value):
    # Attribute reference fields round-trip as {"guid": ...}, everything else as-is.
    if field.endswith('Id') and isinstance(value, dict):
        return value
    return value

def run_full_check(guid, label):
    d = run('GetSectionsAndElevationsDetails', {'elements': [{'elementId': {'guid': guid}}]})
    detail = d['detailsOfElements'][0]
    if 'error' in detail:
        print(f'  ERROR reading back element: {detail}')
        return
    for field, value in ALL_FIELDS.items():
        check(f'[{label}] {field}', expected_value(field, value), detail.get(field))
    # read-only fields must simply be present
    for roField in ('currentPlanConnectivity', 'isWindowOpened', 'isActive', 'isSourceMarker',
                     'isInDrawingMode', 'beginMarkerElementId', 'middleMarkerElementId', 'endMarkerElementId'):
        check(f'[{label}] read-only field present: {roField}', True, roField in detail)
    # Confirmed Archicad limitation (not settable via API): these 4 fields always mirror
    # shadowFill*/modelUncutSurfaceFillType regardless of what was written in 'details'.
    check(f'[{label}] distantAreaShadowFillId mirrors shadowFillId (Archicad limitation)', ALL_FIELDS['shadowFillId'], detail.get('distantAreaShadowFillId'))
    check(f'[{label}] distantAreaShadowFillPenIndex mirrors shadowFillPenIndex (Archicad limitation)', ALL_FIELDS['shadowFillPenIndex'], detail.get('distantAreaShadowFillPenIndex'))
    check(f'[{label}] distantAreaShadowFillBackgroundPenIndex mirrors shadowFillBackgroundPenIndex (Archicad limitation)', ALL_FIELDS['shadowFillBackgroundPenIndex'], detail.get('distantAreaShadowFillBackgroundPenIndex'))
    check(f'[{label}] distantAreaUncutSurfaceFillType always PenColor (Archicad limitation)', 'PenColor', detail.get('distantAreaUncutSurfaceFillType'))

createdGuids = []

print('=' * 70)
print('PATH 1 -- CreateSections with a full "details" block (create-time write path)')
print('=' * 70)
r = run('CreateSections', {'sectionsData': [{
    'startCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 8.0, 'y': 0.0},
    'depth': 2.0,
    'details': ALL_FIELDS,
}]})
print('  create response:', r)
if 'elements' in r and 'elementId' in r['elements'][0]:
    guid = r['elements'][0]['elementId']['guid']
    createdGuids.append(guid)
    run_full_check(guid, 'Create')
else:
    print('  CreateSections with full details FAILED:', r)
    fails += 1
print()

print('=' * 70)
print('PATH 2 -- SetSectionsAndElevationsDetails with a full "details" block on a plain element (modify write path)')
print('=' * 70)
r2 = run('CreateSections', {'sectionsData': [{
    'startCoordinate': {'x': 0.0, 'y': 20.0},
    'endCoordinate': {'x': 8.0, 'y': 20.0},
    'depth': 2.0,
}]})
if 'elements' in r2 and 'elementId' in r2['elements'][0]:
    guid2 = r2['elements'][0]['elementId']['guid']
    createdGuids.append(guid2)
    setResp = run('SetSectionsAndElevationsDetails', {'elementsWithDetails': [{
        'elementId': {'guid': guid2},
        'details': ALL_FIELDS,
    }]})
    print('  set response:', setResp)
    check('SetSectionsAndElevationsDetails succeeds', True, setResp['executionResults'][0].get('success'))
    run_full_check(guid2, 'Set')
else:
    print('  plain CreateSections FAILED:', r2)
    fails += 1
print()

print('=' * 70)
print('PATH 3 -- same full "details" block via CreateElevations (Elevation shares the same struct)')
print('=' * 70)
r3 = run('CreateElevations', {'elevationsData': [{
    'startCoordinate': {'x': 0.0, 'y': 40.0},
    'endCoordinate': {'x': 8.0, 'y': 40.0},
    'depth': 2.0,
    'details': ALL_FIELDS,
}]})
print('  create response:', r3)
if 'elements' in r3 and 'elementId' in r3['elements'][0]:
    guid3 = r3['elements'][0]['elementId']['guid']
    createdGuids.append(guid3)
    run_full_check(guid3, 'Elevation')
else:
    print('  CreateElevations with full details FAILED:', r3)
    fails += 1
print()

if createdGuids:
    run('DeleteElements', {'elements': [{'elementId': {'guid': g}} for g in createdGuids]})

print('=' * 70)
print(f'BILAN :  {passes} PASS  |  {fails} FAIL   (out of {len(ALL_FIELDS)} fields x 3 paths + read-only checks)')
print('=' * 70)
if fails:
    sys.exit(1)
