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

viewSettings = aclib.RunTapirCommand('GetViewSettings', {'navigatorItemIds': views})['viewSettings']

modelViewOptions = aclib.RunTapirCommand('GetModelViewOptions')['modelViewOptions']
layerCombinations = aclib.RunTapirCommand('GetAttributesByType', {'attributeType': 'LayerCombination'})['attributes']

navigatorItemIdsWithViewSettings = []
for i in range(len(viewSettings)):
    oldViewSettings = viewSettings[i]
    newViewSettings = {}
    if 'modelViewOptions' in oldViewSettings:
        newViewSettings['modelViewOptions'] = modelViewOptions[i % len(modelViewOptions)]['name']
    if 'layerCombination' in oldViewSettings:
        newViewSettings['layerCombination'] = layerCombinations[i % len(layerCombinations)]['name']
    if newViewSettings:
        navigatorItemIdsWithViewSettings.append({
                'navigatorItemId': views[i]['navigatorItemId'],
                'viewSettings': newViewSettings
            })

aclib.RunTapirCommand('SetViewSettings', {'navigatorItemIdsWithViewSettings': navigatorItemIdsWithViewSettings})

aclib.RunTapirCommand('GetViewSettings', {'navigatorItemIds': views})