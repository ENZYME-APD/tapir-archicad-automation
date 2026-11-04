"""Makes a View Map view show only the main model with SetDesignOptionCombinationOfViews, then restores its combination.

Needs a View Map view that shows the main model only or a design option combination.
Requires Archicad 27 or later.
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
combinations = aclib.RunTapirCommand ('GetDesignOptionCombinationOfViews', {
    'navigatorItemIds': views
})['designOptionCombinationOfViews']

# Only these two states can be set back exactly.
candidates = [c for c in combinations if c.get ('status') in ('MainModelOnly', 'Standard')]
if not candidates:
    raise SystemExit ('No suitable view in the View Map.')
original = candidates[0]

aclib.PrintJson (aclib.RunTapirCommand ('SetDesignOptionCombinationOfViews', {
    'navigatorItemIdsWithCombination': [{'navigatorItemId': original['navigatorItemId'], 'mainModelOnly': True}]
}))
aclib.RunTapirCommand ('GetDesignOptionCombinationOfViews', {
    'navigatorItemIds': [{'navigatorItemId': original['navigatorItemId']}]
})

if original['status'] == 'Standard':
    aclib.PrintJson (aclib.RunTapirCommand ('SetDesignOptionCombinationOfViews', {
        'navigatorItemIdsWithCombination': [{
            'navigatorItemId': original['navigatorItemId'],
            'designOptionCombinationId': original['designOptionCombinationId']
        }]
    }))
