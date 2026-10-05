import aclib

# Archicad inserts a new layout at the FIRST place of its subset. Layouts are
# numbered by their place in the subset, so every sheet already there gets a
# new number (PK-01 becomes PK-02, ...). CreateLayout's optional position and
# previousNavigatorItemId place the new layout after the existing sheets
# instead, the same way MoveNavigatorItem would.

layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'LayoutBook'
}, debug = False)['navigatorItemTree']

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

def FindItem (item, navigatorItemGuid):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        if navigatorItem['navigatorItemId']['guid'] == navigatorItemGuid:
            return navigatorItem
        found = FindItem (navigatorItem, navigatorItemGuid)
        if found is not None:
            return found
    return None

def ChildNames (parentGuid):
    layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {
        'navigatorMapId': 'LayoutBook'
    }, debug = False)['navigatorItemTree']
    parent = layoutBook if parentGuid is None else FindItem (layoutBook, parentGuid)
    return [child['navigatorItem']['name'] for child in parent.get ('children', [])]

allItems = CollectItems (layoutBook, [])
masterLayout = next (item for item in allItems if item['type'] == 'MasterLayoutItem')

# The layouts go into the first subset of the Layout Book, or to its root when
# the project has no subset. parentNavigatorItemId is left out for the root.
subset = next ((item for item in allItems if item['type'] == 'SubSetItem'), None)
parentGuid = subset['navigatorItemId']['guid'] if subset is not None else None
parentParameters = {'parentNavigatorItemId': subset['navigatorItemId']} if subset is not None else {}

print ('Sheets before: {}'.format (ChildNames (parentGuid)))

# position "last" appends each new layout after the sheets already in the
# subset, so a batch of them ends up in the given order at the end.
response = aclib.RunTapirCommand ('CreateLayout', {
    'layoutsData': [
        dict (parentParameters, layoutName = 'Visualization 1', masterNavigatorItemId = masterLayout['navigatorItemId'], position = 'last'),
        dict (parentParameters, layoutName = 'Visualization 2', masterNavigatorItemId = masterLayout['navigatorItemId'], position = 'last'),
    ]
})
assert all ('databaseId' in database for database in response['databases']), response

sheetsAfterAppend = ChildNames (parentGuid)
print ('Sheets after appending two layouts: {}'.format (sheetsAfterAppend))
assert sheetsAfterAppend[-2:] == ['Visualization 1', 'Visualization 2'], sheetsAfterAppend

# previousNavigatorItemId puts the new layout right after a given sheet of the
# same subset: here between the two layouts created above.
layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'LayoutBook'
}, debug = False)['navigatorItemTree']
createdLayouts = [item for item in CollectItems (layoutBook, []) if item['name'] in ('Visualization 1', 'Visualization 2')]
firstCreated = next (item for item in createdLayouts if item['name'] == 'Visualization 1')

response = aclib.RunTapirCommand ('CreateLayout', {
    'layoutsData': [
        dict (parentParameters, layoutName = 'Visualization 1b', masterNavigatorItemId = masterLayout['navigatorItemId'], previousNavigatorItemId = firstCreated['navigatorItemId']),
    ]
})
assert 'databaseId' in response['databases'][0], response

sheetsAfterInsert = ChildNames (parentGuid)
print ('Sheets after inserting after Visualization 1: {}'.format (sheetsAfterInsert))
assert sheetsAfterInsert[-3:] == ['Visualization 1', 'Visualization 1b', 'Visualization 2'], sheetsAfterInsert

# Clean up: remove the layouts created by this example.
layoutBook = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'LayoutBook'
}, debug = False)['navigatorItemTree']
createdLayouts = [item for item in CollectItems (layoutBook, []) if item['name'] in ('Visualization 1', 'Visualization 1b', 'Visualization 2')]
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': item['navigatorItemId']} for item in createdLayouts]
}, debug = False)
print ('Sheets after clean-up: {}'.format (ChildNames (parentGuid)))
