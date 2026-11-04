"""Gets the database of each layout of the Layout Book with GetDatabaseIdFromNavigatorItemId."""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
layouts = [item for item in AllItems (layoutBook) if item['type'] == 'LayoutItem']
if not layouts:
    raise SystemExit ('No layout in the Layout Book.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDatabaseIdFromNavigatorItemId', {
    'navigatorItemIds': [{'navigatorItemId': layout['navigatorItemId']} for layout in layouts]
}))
