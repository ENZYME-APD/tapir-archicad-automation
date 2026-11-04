"""Prints the view settings (layer combination, scale, pen set, ...) of the View Map views with GetViewSettings."""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

viewMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'PublicViewMap'})['navigatorItemTree']
views = [item for item in AllItems (viewMap) if item['type'] != 'FolderItem']
if not views:
    raise SystemExit ('No view in the View Map.')

aclib.PrintJson (aclib.RunTapirCommand ('GetViewSettings', {
    'navigatorItemIds': [{'navigatorItemId': view['navigatorItemId']} for view in views]
}))
