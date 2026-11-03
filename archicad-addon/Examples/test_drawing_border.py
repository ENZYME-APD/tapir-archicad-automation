import json
import aclib

# Never runs against an arbitrary layout it happens to find - this script can be pointed at a
# real, in-use project, so it only ever touches a layout with this exact name, and refuses to run
# otherwise. Create a layout with this name and place any drawing on it before running.
TEST_LAYOUT_NAME = 'TAPIR_TEST_DRAWING_BORDER'

BORDER_FIELDS = ['hasBorderLine', 'borderLineTypeIndex', 'borderPenIndex', 'borderSize']


def find_layout_by_name(item, name):
    navItem = item.get('navigatorItem', item)
    if navItem.get('type') == 'LayoutItem' and navItem.get('name') == name:
        return navItem['navigatorItemId']
    for c in navItem.get('children', []):
        found = find_layout_by_name(c, name)
        if found:
            return found
    return None


def get_border(drawingId):
    resp = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [{'elementId': drawingId}]}, debug=False)
    details = resp['detailsOfElements'][0]['details']
    return {field: details.get(field) for field in BORDER_FIELDS}


def set_border(drawingId, border):
    return aclib.RunTapirCommand('SetDetailsOfElements', {
        'elementsWithDetails': [{'elementId': drawingId, 'details': {'typeSpecificDetails': border}}]
    }, debug=False)


print('Looking for the "{}" test layout...'.format(TEST_LAYOUT_NAME))
lb = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layoutNavId = find_layout_by_name(lb['navigatorTree']['rootItem'], TEST_LAYOUT_NAME)
if layoutNavId is None:
    print('No layout named "{}" found - create one (with any drawing placed on it) and rerun.'.format(TEST_LAYOUT_NAME))
    raise SystemExit

aclib.RunTapirCommand('ChangeWindow', {'windowType': 'Layout', 'navigatorItemId': layoutNavId}, debug=False)

resp = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Drawing'}, debug=False)
drawings = resp['elements'] if resp else []
if not drawings:
    print('No drawings on the test layout - place one and rerun.')
    raise SystemExit

drawingId = drawings[0]['elementId']
print('Using drawing: {}'.format(drawingId))

before = get_border(drawingId)
print('Before: {}'.format(json.dumps(before)))

# Office standard: printable border on, solid line (index 1), pen 2, 2 mm.
wanted = {'hasBorderLine': True, 'borderLineTypeIndex': 1, 'borderPenIndex': 2, 'borderSize': 0.002}
print('SetDetailsOfElements response: {}'.format(json.dumps(set_border(drawingId, wanted))))

after = get_border(drawingId)
print('After: {}'.format(json.dumps(after)))
ok = (after['hasBorderLine'] is True and after['borderLineTypeIndex'] == 1 and after['borderPenIndex'] == 2
      and abs(after['borderSize'] - 0.002) < 1e-9)

print('Restoring original border...')
# Index fields only accept valid (positive) indices, so an unset one is left as it is.
set_border(drawingId, {k: v for k, v in before.items()
                       if v is not None and not (k in ('borderLineTypeIndex', 'borderPenIndex') and v < 1)})

print('\n{}'.format('BORDER TEST PASSED' if ok else 'BORDER TEST FAILED'))
