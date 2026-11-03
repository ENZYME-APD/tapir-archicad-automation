import aclib

# GetNavigatorItemTree with includeChildCount adds the number of direct
# children Archicad reports to every item.
navigatorItemTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'PublicViewMap',
    'includeChildCount': True
})['navigatorItemTree']

def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems

items = CollectItems (navigatorItemTree, [])

print ('{} has {} direct children'.format (
    navigatorItemTree['name'],
    navigatorItemTree.get ('childCount')))
for item in items:
    if item['type'] == 'FolderItem':
        print ('  folder {} has {} direct children'.format (item['name'], item.get ('childCount')))

# GetNavigatorParentItems walks up the tree: it returns the folder (or the
# layout book subset, or the map root) each item is in. The root itself has no
# parent and gets an error.
views = [item for item in items if item['type'] != 'FolderItem']
parentItems = aclib.RunTapirCommand ('GetNavigatorParentItems', {
    'navigatorItemIds': [{'navigatorItemId': item['navigatorItemId']} for item in views]
         + [{'navigatorItemId': navigatorItemTree['navigatorItemId']}]
})['parentItems']

for view, parentItem in zip (views, parentItems):
    if 'error' in parentItem:
        print ('{}: {}'.format (view['name'], parentItem['error']['message']))
    else:
        print ('{} is in {} ({})'.format (view['name'], parentItem['name'], parentItem['type']))

rootParent = parentItems[-1]
print ('Parent of the root: {}'.format (
    rootParent['error']['message'] if 'error' in rootParent else rootParent['name']))
