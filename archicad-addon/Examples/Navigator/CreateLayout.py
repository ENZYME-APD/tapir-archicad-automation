"""Creates a layout at the end of the Layout Book with CreateLayout, then deletes it again.

Needs at least one master layout.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
masters = [item for item in AllItems (layoutBook) if item['type'] == 'MasterLayoutItem']
if not masters:
    raise SystemExit ('No master layout in the Layout Book.')

layoutName = 'Tapir Example Layout'
aclib.PrintJson (aclib.RunTapirCommand ('CreateLayout', {
    'layoutsData': [{
        'layoutName': layoutName,
        'masterNavigatorItemId': masters[0]['navigatorItemId'],
        'position': 'last'
    }]
}))

# Clean up: find the new layout in the Layout Book and delete it.
layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
created = [item for item in AllItems (layoutBook) if item['type'] == 'LayoutItem' and item['name'] == layoutName]
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': item['navigatorItemId']} for item in created]
})
