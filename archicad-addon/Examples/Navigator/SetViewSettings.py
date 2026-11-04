"""Changes the Partial Structure Display of the first View Map view with SetViewSettings, then restores it.

Needs a View Map view that stores a structure display.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

viewMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'PublicViewMap'})['navigatorItemTree']
views = [{'navigatorItemId': item['navigatorItemId']} for item in AllItems (viewMap) if item['type'] != 'FolderItem']
if not views:
    raise SystemExit ('No view in the View Map.')
viewSettings = aclib.RunTapirCommand ('GetViewSettings', {'navigatorItemIds': views})['viewSettings']

candidates = [(view, settings) for view, settings in zip (views, viewSettings) if 'structureDisplay' in settings]
if not candidates:
    raise SystemExit ('No View Map view with a structure display.')
view, settings = candidates[0]

newDisplay = 'CoreOnly' if settings['structureDisplay'] != 'CoreOnly' else 'EntireStructure'
aclib.PrintJson (aclib.RunTapirCommand ('SetViewSettings', {
    'navigatorItemIdsWithViewSettings': [{
        'navigatorItemId': view['navigatorItemId'],
        'viewSettings': {'structureDisplay': newDisplay}
    }]
}))
aclib.RunTapirCommand ('GetViewSettings', {'navigatorItemIds': [view]})

aclib.PrintJson (aclib.RunTapirCommand ('SetViewSettings', {
    'navigatorItemIdsWithViewSettings': [{
        'navigatorItemId': view['navigatorItemId'],
        'viewSettings': {'structureDisplay': settings['structureDisplay']}
    }]
}))
