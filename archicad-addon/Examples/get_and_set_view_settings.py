import aclib

def get_navigator_item_guids_from_tree(current_branch):
    guids = []
    for navigator_item in current_branch:
        navigator_item = navigator_item['navigatorItem']
        guids.append(navigator_item['navigatorItemId'])
        children = navigator_item.get('children')
        if children:
            guids.extend(get_navigator_item_guids_from_tree(children))
    return guids

navigator_tree = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ViewMap'}})

viewItems = [{'navigatorItemId': guid} for guid in get_navigator_item_guids_from_tree(navigator_tree['navigatorTree']['rootItem']['children'])]

viewSettings = aclib.RunTapirCommand('GetNavigatorViews', {'navigatorItemIds': viewItems})['navigatorViews']

modelViewOptions = list(set([viewSetting['modelViewOptions'] for viewSetting in viewSettings if 'modelViewOptions' in viewSetting]))
layerCombinations = list(set([viewSetting['layerCombination'] for viewSetting in viewSettings if 'layerCombination' in viewSetting]))

navigatorItemIdsWithViewSettings = []
for i in range(len(viewSettings)):
    oldViewSettings = viewSettings[i]
    newViewSettings = {}
    if 'modelViewOptions' in oldViewSettings:
        newViewSettings['modelViewOptions'] = modelViewOptions[i % len(modelViewOptions)]
    if 'layerCombination' in oldViewSettings:
        newViewSettings['layerCombination'] = layerCombinations[i % len(layerCombinations)]
    if newViewSettings:
        navigatorItemIdsWithViewSettings.append({
                'navigatorItemId': viewItems[i]['navigatorItemId'],
                'navigatorView': newViewSettings
            })

aclib.RunTapirCommand('SetNavigatorViews', {'navigatorItemIdsWithViewSettings': navigatorItemIdsWithViewSettings})

aclib.RunTapirCommand('GetNavigatorViews', {'navigatorItemIds': viewItems})