import aclib

# RenameNavigatorItem's newId sets the ID shown next to the name on the
# navigator. For a view it becomes a custom ID - the View Settings "Custom"
# radio button - so the view no longer follows its Project Map source; for a
# layout it is the custom layout number. The example works on a view of its
# own: it clones the first story into the View Map, gives the clone an ID,
# reads it back with GetNavigatorItemTree and deletes the clone again.

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

def FindItem (navigatorMapId, navigatorItemId):
    navigatorItemTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
        'navigatorMapId': navigatorMapId
    }, debug = False)['navigatorItemTree']
    for item in CollectItems (navigatorItemTree, []):
        if item['navigatorItemId']['guid'] == navigatorItemId['guid']:
            return item
    return None

projectMapTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'ProjectMap'
}, debug = False)['navigatorItemTree']
story = next (item for item in CollectItems (projectMapTree, []) if item['type'] == 'StoryItem')

clonedViewId = aclib.RunTapirCommand ('CloneProjectMapItemToViewMap', {
    'viewsData': [{ 'navigatorItemId': story['navigatorItemId'] }]
}, debug = False)['navigatorItems'][0]['navigatorItemId']

view = FindItem ('PublicViewMap', clonedViewId)
print ('Before: ID "{}", custom ID: {}'.format (view['uiId'], view['customUiId']))

# A newId that Archicad does not store on the given item fails instead of
# reporting a success that changed nothing.
aclib.RunTapirCommand ('RenameNavigatorItem', {
    'navigatorItemId': clonedViewId,
    'newId': 'A-101'
})

view = FindItem ('PublicViewMap', clonedViewId)
print ('After:  ID "{}", custom ID: {}'.format (view['uiId'], view['customUiId']))

aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{ 'navigatorItemId': clonedViewId }]
}, debug = False)
