import aclib

# RenameNavigatorItem renames View Map views and Layout Book items directly. A
# Project Map section, elevation, interior elevation, detail or worksheet has no
# name of its own: the viewpoint shows the name of the element behind it, so the
# command renames that element. Either way the new name is verified by reading
# the item back, and the command fails instead of reporting a rename Archicad
# silently dropped (see issue #755).

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

def GetProjectMapItems ():
    tree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
        'navigatorMapId': 'ProjectMap'
    }, debug = False)['navigatorItemTree']
    return CollectItems (tree, [])

def FindItem (items, navigatorItemId):
    return next ((item for item in items if item['navigatorItemId']['guid'] == navigatorItemId['guid']), None)

renamableTypes = ['SectionItem', 'ElevationItem', 'InteriorElevationItem', 'DetailDrawingItem', 'WorksheetDrawingItem']
candidates = [item for item in GetProjectMapItems () if item['type'] in renamableTypes]
if not candidates:
    print ('No section, elevation, interior elevation, detail or worksheet found in the Project Map.')
else:
    item = candidates[0]
    originalName = item['name']
    newName = originalName + ' (renamed by Tapir)'
    print ('Renaming {} "{}" to "{}"'.format (item['type'], originalName, newName))

    response = aclib.RunTapirCommand ('RenameNavigatorItem', {
        'navigatorItemId': item['navigatorItemId'],
        'newName': newName
    })

    # A fresh read of the Project Map must show the new name on the same item.
    renamedItem = FindItem (GetProjectMapItems (), item['navigatorItemId'])
    print ('Name after rename: "{}"'.format (renamedItem['name']))

    if response.get ('success'):
        # Restore the original name so the example leaves the project unchanged.
        aclib.RunTapirCommand ('RenameNavigatorItem', {
            'navigatorItemId': item['navigatorItemId'],
            'newName': originalName
        }, debug = False)
        restoredItem = FindItem (GetProjectMapItems (), item['navigatorItemId'])
        print ('Name after restore: "{}"'.format (restoredItem['name']))
