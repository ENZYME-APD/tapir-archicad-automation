import aclib


def findFirstLayout(branch):
    for entry in branch:
        navigatorItem = entry['navigatorItem']
        if navigatorItem.get('type') == 'LayoutItem':
            return navigatorItem
        children = navigatorItem.get('children')
        if children:
            found = findFirstLayout(children)
            if found is not None:
                return found
    return None


# The drawing numbering grid belongs to a layout, so the first layout of the Layout Book is
# made the current window first.
layoutBookTree = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
firstLayout = findFirstLayout(layoutBookTree['navigatorTree']['rootItem']['children'])

if firstLayout is None:
    print('No layouts found in the Layout Book.')
else:
    databases = aclib.RunTapirCommand(
        'GetDatabaseIdFromNavigatorItemId',
        {
            'navigatorItemIds': [{'navigatorItemId': firstLayout['navigatorItemId']}]
        }
    )['databases']
    aclib.RunTapirCommand('ChangeWindow', {'databaseId': databases[0]['databaseId']})

    # Only a layout that uses grid-based drawing numbering has a grid.
    lines = aclib.RunTapirCommand('GetDrawingNumberingGridLines').get('lines', [])
    print(f"{len(lines)} grid line(s) on layout {firstLayout.get('name', '<unnamed>')}")
