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
    print('No saved views found in the View Map; nothing to go to.')
else:
    print(f"Going to view: {firstView.get('name', '<unnamed>')}")
    result = aclib.RunTapirCommand('GoToView', {'navigatorItemId': firstView['navigatorItemId']})
    print(result)
