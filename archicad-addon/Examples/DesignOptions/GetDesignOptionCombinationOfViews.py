"""Prints which design option combination each View Map view shows, with GetDesignOptionCombinationOfViews.

Requires Archicad 27 or later.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

viewMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'PublicViewMap'})['navigatorItemTree']
views = [item for item in AllItems (viewMap) if item['type'] != 'FolderItem']
if not views:
    raise SystemExit ('No view in the View Map.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDesignOptionCombinationOfViews', {
    'navigatorItemIds': [{'navigatorItemId': view['navigatorItemId']} for view in views]
}))
