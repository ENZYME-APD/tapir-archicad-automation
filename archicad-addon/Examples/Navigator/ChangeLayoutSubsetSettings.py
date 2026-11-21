"""Changes the ID prefix and numbering of a Layout Book subset with ChangeLayoutSubsetSettings.

Creates a subset with one layout first. The command moves the layout into a new subset with the new
settings and leaves the old subset empty; the layout and its master are deleted at the end, the two
subsets stay in the Layout Book: subsets cannot be deleted through the Archicad API.
"""
import aclib

subset = aclib.RunTapirCommand ('CreateLayoutSubset', {
    'subsetsData': [{'name': 'Tapir Example Subset', 'addOwnPrefix': True, 'ownPrefix': 'TA', 'numberingStyle': '001', 'startAt': 1}]
})['navigatorItems'][0]
layout = aclib.RunTapirCommand ('CreateLayout', {
    'layoutsData': [{'layoutName': 'Tapir Example Layout', 'masterLayoutName': 'Tapir Example Master', 'parentNavigatorItemId': subset['navigatorItemId']}]
})['databases'][0]

changed = aclib.RunTapirCommand ('ChangeLayoutSubsetSettings', {
    'subsets': [{'navigatorItemId': subset['navigatorItemId'], 'settings': {'ownPrefix': 'TB', 'numberingStyle': '01', 'startAt': 10}}]
})['subsets'][0]
aclib.PrintJson (changed)
aclib.PrintJson (aclib.RunTapirCommand ('GetLayoutSubsetSettings', {'navigatorItemIds': [{'navigatorItemId': changed['navigatorItemId']}]}))

# The layout and its master, created above, are deleted again.
layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
master = aclib.RunTapirCommand ('GetLayoutSettings', {'layoutDatabaseIds': [{'databaseId': layout['databaseId']}]})['layoutSettings'][0]['masterNavigatorItemId']
newSubsetGuid = changed['navigatorItemId']['guid'].lower ()
newSubsetItems = [child['navigatorItem'] for child in layoutBook.get ('children', [])
                  if child['navigatorItem']['navigatorItemId']['guid'].lower () == newSubsetGuid]
layouts = newSubsetItems[0].get ('children', []) if newSubsetItems else []
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': child['navigatorItem']['navigatorItemId']} for child in layouts] + [{'navigatorItemId': master}]
})
