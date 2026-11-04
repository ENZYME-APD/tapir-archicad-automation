"""Prints the folder each View Map view is in, with GetNavigatorParentItems."""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

viewMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'PublicViewMap'})['navigatorItemTree']
views = [item for item in AllItems (viewMap) if item['type'] != 'FolderItem']
if not views:
    raise SystemExit ('No view in the View Map.')

parents = aclib.RunTapirCommand ('GetNavigatorParentItems', {
    'navigatorItemIds': [{'navigatorItemId': view['navigatorItemId']} for view in views]
})['parentItems']

for view, parent in zip (views, parents):
    print (view['name'], '->', parent.get ('name', parent.get ('error')))
