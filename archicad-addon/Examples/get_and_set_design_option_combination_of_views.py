import aclib


def getNavigatorItemIdsFromTree(branch):
    navigatorItemIds = []
    for navigatorItem in branch:
        navigatorItem = navigatorItem['navigatorItem']
        navigatorItemIds.append({'navigatorItemId': navigatorItem['navigatorItemId']})
        children = navigatorItem.get('children')
        if children:
            navigatorItemIds.extend(getNavigatorItemIdsFromTree(children))
    return navigatorItemIds


viewMapTree = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ViewMap'}})
views = getNavigatorItemIdsFromTree(viewMapTree['navigatorTree']['rootItem']['children'])

combinationOfViews = aclib.RunTapirCommand('GetDesignOptionCombinationOfViews', {'navigatorItemIds': views})['designOptionCombinationOfViews']

designOptionCombinations = aclib.RunTapirCommand('GetDesignOptionCombinations')['designOptionCombinations']

# Show only the main model in every view, then switch the views to the first combination, if there is any
viewsWithCombination = [view for view, combination in zip(views, combinationOfViews) if 'error' not in combination]
aclib.RunTapirCommand('SetDesignOptionCombinationOfViews', {
    'navigatorItemIdsWithCombination': [{'navigatorItemId': view['navigatorItemId'], 'mainModelOnly': True} for view in viewsWithCombination]
})
aclib.RunTapirCommand('GetDesignOptionCombinationOfViews', {'navigatorItemIds': viewsWithCombination})

if designOptionCombinations:
    aclib.RunTapirCommand('SetDesignOptionCombinationOfViews', {
        'navigatorItemIdsWithCombination': [{
            'navigatorItemId': view['navigatorItemId'],
            'designOptionCombinationId': designOptionCombinations[0]['designOptionCombinationId']
        } for view in viewsWithCombination]
    })
    aclib.RunTapirCommand('GetDesignOptionCombinationOfViews', {'navigatorItemIds': viewsWithCombination})
