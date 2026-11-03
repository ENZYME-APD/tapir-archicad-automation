import aclib


def CollectItems (item, collectedItems):
    for child in item.get ('children', []):
        navigatorItem = child['navigatorItem']
        collectedItems.append (navigatorItem)
        CollectItems (navigatorItem, collectedItems)
    return collectedItems


# ReorderNavigatorItems needs a parent whose children can be bounced through
# one another - at least one of them must itself be able to host items
# (a folder or subset), and the Project Map works for this; find its first
# folder with at least two children.
projectMapTree = aclib.RunTapirCommand ('GetNavigatorItemTree', {
    'navigatorMapId': 'ProjectMap'
})['navigatorItemTree']

folders = [
    item for item in CollectItems (projectMapTree, [])
    if item['type'] == 'FolderItem' and len (item.get ('children', [])) >= 2
]
if not folders:
    print ('No folder with at least two children found in the Project Map - nothing to reorder.')
else:
    folder = folders[0]
    children = [child['navigatorItem'] for child in folder['children']]
    print ('Folder: {} ({} children)'.format (folder['name'], len (children)))
    for child in children:
        print ('  {}'.format (child['name']))

    # Reverse the current order, then restore it - a no-op overall, but
    # exercises the command on a folder/subset that already has real content
    # instead of needing to create throwaway items first.
    reversedOrder = list (reversed (children))

    reorderResult = aclib.RunTapirCommand ('ReorderNavigatorItems', {
        'parentNavigatorItemId': folder['navigatorItemId'],
        'orderedNavigatorItemIds': [
            {'navigatorItemId': child['navigatorItemId']} for child in reversedOrder
        ]
    })
    print ('Reversed: {}'.format (reorderResult))

    restoreResult = aclib.RunTapirCommand ('ReorderNavigatorItems', {
        'parentNavigatorItemId': folder['navigatorItemId'],
        'orderedNavigatorItemIds': [
            {'navigatorItemId': child['navigatorItemId']} for child in children
        ]
    })
    print ('Restored: {}'.format (restoreResult))
