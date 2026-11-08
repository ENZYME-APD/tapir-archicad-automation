import json
import aclib

# Round trip of the Drawing Settings > "Include scale in Autotext and index" checkbox
# (isScaleIncludedInAutotextAndIndex) through GetDetailsOfElements / SetDetailsOfElements (#817).
#
# Never runs against an arbitrary layout it happens to find - this script can be pointed at a
# real, in-use project, so it only ever touches a layout with this exact name, and refuses to run
# otherwise. Create a layout with this name and place any drawing on it before running.
TEST_LAYOUT_NAME = 'TAPIR_TEST_DRAWING_SCALE_AUTOTEXT'


def find_layout_by_name(item, name):
    navItem = item.get('navigatorItem', item)
    if navItem.get('type') == 'LayoutItem' and navItem.get('name') == name:
        return navItem['navigatorItemId']
    for c in navItem.get('children', []):
        found = find_layout_by_name(c, name)
        if found:
            return found
    return None


def get_flag(drawingId):
    resp = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [{'elementId': drawingId}]}, debug=False)
    return resp['detailsOfElements'][0]['details']['isScaleIncludedInAutotextAndIndex']


def set_flag(drawingId, value):
    return aclib.RunTapirCommand('SetDetailsOfElements', {
        'elementsWithDetails': [{
            'elementId': drawingId,
            'details': {'typeSpecificDetails': {'isScaleIncludedInAutotextAndIndex': value}}
        }]
    }, debug=False)


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

before = get_flag(drawingId)
print('Before: isScaleIncludedInAutotextAndIndex={}'.format(before))

setResp = set_flag(drawingId, not before)
print('SetDetailsOfElements response: {}'.format(json.dumps(setResp)))

after = get_flag(drawingId)
print('After: isScaleIncludedInAutotextAndIndex={}'.format(after))

flipOk = (after is (not before))

print('Restoring original value...')
set_flag(drawingId, before)
restored = get_flag(drawingId)
print('Restored: isScaleIncludedInAutotextAndIndex={}'.format(restored))

print('\n{}'.format('SCALE AUTOTEXT TEST PASSED' if flipOk and restored is before else 'SCALE AUTOTEXT TEST FAILED'))
