import aclib


def findFirstLeafView(branch):
    for entry in branch:
        navigatorItem = entry['navigatorItem']
        children = navigatorItem.get('children')
        if children:
            found = findFirstLeafView(children)
            if found is not None:
                return found
        else:
            return navigatorItem
    return None


viewMapTree = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ViewMap'}})
firstView = findFirstLeafView(viewMapTree['navigatorTree']['rootItem']['children'])

if firstView is None:
    print('No saved views found in the View Map; nothing to change to.')
else:
    databases = aclib.RunTapirCommand(
        'GetDatabaseIdFromNavigatorItemId',
        {
            'navigatorItemIds': [{'navigatorItemId': firstView['navigatorItemId']}]
        }
    )['databases']

    if 'databaseId' not in databases[0]:
        print(f"View {firstView.get('name', '<unnamed>')} has no database.")
    else:
        # windowType can be omitted: the window type is taken from the database.
        result = aclib.RunTapirCommand(
            'ChangeWindow',
            {
                'databaseId': databases[0]['databaseId']
            }
        )
        print(result)
