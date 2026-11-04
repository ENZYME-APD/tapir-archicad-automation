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
    print(f"Changing to view: {firstView.get('name', '<unnamed>')}")
    # navigatorItemId routes through ACAPI_View_GoToView (AC27+), applying
    # the view's saved layer combination, scale, MVO, dim, and zoom.
    # On AC25/26 navigatorItemId returns NOTSUPPORTED; use databaseId there.
    result = aclib.RunTapirCommand(
        'ChangeWindow',
        {
            'navigatorItemId': firstView['navigatorItemId'],
        }
    )
    print(result)
