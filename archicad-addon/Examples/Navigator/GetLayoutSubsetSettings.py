"""Prints the ID prefix and numbering settings of every Layout Book subset with GetLayoutSubsetSettings."""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
subsets = [item for item in AllItems (layoutBook) if item['type'] == 'SubSetItem']
if not subsets:
    raise SystemExit ('No subset in the Layout Book.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLayoutSubsetSettings', {
    'navigatorItemIds': [{'navigatorItemId': subset['navigatorItemId']} for subset in subsets]
}))
