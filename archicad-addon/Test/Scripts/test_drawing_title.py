import json
import aclib

# Never runs against an arbitrary layout it happens to find - this script can be pointed at a
# real, in-use project, so it only ever touches a layout with this exact name, and refuses to run
# otherwise. Create a layout with this name and place any drawing on it before running.
TEST_LAYOUT_NAME = 'TAPIR_TEST_DRAWING_TITLE'


def find_layout_by_name(item, name):
    navItem = item.get('navigatorItem', item)
    if navItem.get('type') == 'LayoutItem' and navItem.get('name') == name:
        return navItem['navigatorItemId']
    for c in navItem.get('children', []):
        found = find_layout_by_name(c, name)
        if found:
            return found
    return None


print('Looking for the "{}" test layout...'.format(TEST_LAYOUT_NAME))
lb = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layoutNavId = find_layout_by_name(lb['navigatorTree']['rootItem'], TEST_LAYOUT_NAME)
if layoutNavId is None:
    print('No layout named "{}" found - create one (with any drawing placed on it) and rerun.'.format(TEST_LAYOUT_NAME))
    raise SystemExit

aclib.RunTapirCommand('ChangeWindow', {'windowType': 'Layout', 'navigatorItemId': layoutNavId}, debug=False)

print('Looking for a Drawing element on the test layout...')
resp = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Drawing'}, debug=False)
drawings = resp['elements'] if resp else []
print('Found {} drawing(s).'.format(len(drawings)))

if not drawings:
    print('No drawings on the test layout - place one and rerun.')
    raise SystemExit

drawingId = drawings[0]['elementId']
print('Using drawing: {}'.format(drawingId))

# --- Part 1: name/numbering fields (round trip on the test drawing, restored afterwards) ---

beforeResp = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [{'elementId': drawingId}]}, debug=False)
beforeDetails = beforeResp['detailsOfElements'][0]['details']
print('Before: nameType={}, customName={}, numberingType={}, customNumber={}, isInNumbering={}'.format(
    beforeDetails.get('nameType'), beforeDetails.get('customName'),
    beforeDetails.get('numberingType'), beforeDetails.get('customNumber'),
    beforeDetails.get('isInNumbering')))

setResp = aclib.RunTapirCommand('SetDetailsOfElements', {
    'elementsWithDetails': [{
        'elementId': drawingId,
        'details': {
            'typeSpecificDetails': {
                'nameType': 'CustomName',
                'customName': 'SALE PLAN TEST',
                'numberingType': 'CustomNumber',
                'customNumber': '99',
                'isInNumbering': False
            }
        }
    }]
}, debug=False)
print('SetDetailsOfElements response: {}'.format(json.dumps(setResp)))

afterResp = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [{'elementId': drawingId}]}, debug=False)
afterDetails = afterResp['detailsOfElements'][0]['details']
print('After: nameType={}, customName={}, numberingType={}, customNumber={}, isInNumbering={}'.format(
    afterDetails.get('nameType'), afterDetails.get('customName'),
    afterDetails.get('numberingType'), afterDetails.get('customNumber'),
    afterDetails.get('isInNumbering')))

namingOk = (afterDetails.get('nameType') == 'CustomName' and afterDetails.get('customName') == 'SALE PLAN TEST'
            and afterDetails.get('numberingType') == 'CustomNumber' and afterDetails.get('customNumber') == '99'
            and afterDetails.get('isInNumbering') is False)

print('Restoring original name/numbering fields...')
restoreDetails = {
    'nameType': beforeDetails.get('nameType'),
    'numberingType': beforeDetails.get('numberingType'),
    'isInNumbering': beforeDetails.get('isInNumbering'),
}
if beforeDetails.get('customName') is not None:
    restoreDetails['customName'] = beforeDetails['customName']
if beforeDetails.get('customNumber') is not None:
    restoreDetails['customNumber'] = beforeDetails['customNumber']
aclib.RunTapirCommand('SetDetailsOfElements', {
    'elementsWithDetails': [{'elementId': drawingId, 'details': {'typeSpecificDetails': restoreDetails}}]
}, debug=False)

print('\n{}'.format('NAME/NUMBERING TEST PASSED' if namingOk else 'NAME/NUMBERING TEST FAILED'))

# --- Part 2: title object (titleLibraryPartIndex / titleElementId / its own GDL parameters) ---
# The drawing title is a separate placed GDL object (API_DrawingTitle) - a negative/missing
# titleLibraryPartIndex means no title object exists yet. If any OTHER drawing in the project
# already has one, copy its library part index and GDL parameters onto our test drawing to
# verify both the read side and the write side of that mechanism. The test drawing's own
# previous title state (if any) is restored afterwards.

print('\nLooking for a drawing (other than the test one) that already has a title object...')
allDrawingsResp = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Drawing'}, debug=False)
beforeTitleLibInd = beforeDetails.get('titleLibraryPartIndex')

referenceTitleLibInd = None
referenceTitleElementId = None
# Search across all layouts, since a suitable reference title is unlikely to be on the test
# layout itself.
lbTree = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})


def collect_layouts(item, result):
    navItem = item.get('navigatorItem', item)
    if navItem.get('type') == 'LayoutItem':
        result.append(navItem)
    for c in navItem.get('children', []):
        collect_layouts(c, result)
    return result


for layoutItem in collect_layouts(lbTree['navigatorTree']['rootItem'], []):
    if layoutItem['name'] == TEST_LAYOUT_NAME:
        continue
    aclib.RunTapirCommand('ChangeWindow', {'windowType': 'Layout', 'navigatorItemId': layoutItem['navigatorItemId']},
                           debug=False)
    otherDrawings = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Drawing'}, debug=False).get('elements', [])
    if not otherDrawings:
        continue
    otherDetails = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': otherDrawings}, debug=False)['detailsOfElements']
    found = next((d['details'] for d in otherDetails if d.get('details', {}).get('titleElementId')), None)
    if found:
        referenceTitleLibInd = found['titleLibraryPartIndex']
        referenceTitleElementId = found['titleElementId']
        break

aclib.RunTapirCommand('ChangeWindow', {'windowType': 'Layout', 'navigatorItemId': layoutNavId}, debug=False)

if referenceTitleElementId is None:
    print('No other drawing with an existing title object found in this project - skipping title object test.')
else:
    print('Reference title: libraryPartIndex={}, elementId={}'.format(referenceTitleLibInd, referenceTitleElementId))

    refGdlResp = aclib.RunTapirCommand('GetGDLParametersOfElements',
                                        {'elements': [{'elementId': referenceTitleElementId}]}, debug=False)
    refParams = refGdlResp['gdlParametersOfElements'][0].get('parameters', [])
    print('Reference title has {} GDL parameter(s).'.format(len(refParams)))

    # AC_BackReference* is an internal link specific to the instance it came from (which drawing
    # it annotates) and must not be copied; array-valued parameters aren't accepted by
    # SetGDLParametersOfElements (it expects a single scalar value per parameter).
    copyableParams = [
        {'name': p['name'], 'value': p['value']} for p in refParams
        if 'value' in p and not isinstance(p['value'], list) and not p['name'].startswith('AC_BackReference')
    ]
    print('{} parameter(s) are copyable.'.format(len(copyableParams)))

    setTitleResp = aclib.RunTapirCommand('SetDetailsOfElements', {
        'elementsWithDetails': [{
            'elementId': drawingId,
            'details': {'typeSpecificDetails': {'titleLibraryPartIndex': referenceTitleLibInd}}
        }]
    }, debug=False)
    print('SetDetailsOfElements (titleLibraryPartIndex) response: {}'.format(json.dumps(setTitleResp)))

    afterTitleResp = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [{'elementId': drawingId}]}, debug=False)
    afterTitleDetails = afterTitleResp['detailsOfElements'][0]['details']
    newTitleElementId = afterTitleDetails.get('titleElementId')
    print('Test drawing now has titleElementId: {}'.format(newTitleElementId))

    titleObjectCreated = newTitleElementId is not None
    gdlCopyOk = False
    if titleObjectCreated and copyableParams:
        setGdlResp = aclib.RunTapirCommand('SetGDLParametersOfElements', {
            'elementsWithGDLParameters': [{
                'elementId': newTitleElementId,
                'gdlParameters': copyableParams
            }]
        }, debug=False)
        gdlCopyOk = (setGdlResp.get('executionResults') or [{}])[0].get('success', False)
        print('SetGDLParametersOfElements on new title response: {}'.format(json.dumps(setGdlResp)))

    print('Restoring test drawing title library part index...')
    aclib.RunTapirCommand('SetDetailsOfElements', {
        'elementsWithDetails': [{
            'elementId': drawingId,
            'details': {'typeSpecificDetails': {'titleLibraryPartIndex': beforeTitleLibInd if beforeTitleLibInd is not None else -1}}
        }]
    }, debug=False)

    print('\n{}'.format('TITLE OBJECT TEST PASSED' if titleObjectCreated and gdlCopyOk else 'TITLE OBJECT TEST FAILED'))
