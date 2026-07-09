import math
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
dimensionStyle = "US Architect"
penSetName = "11 Archicad 9"
graphicalOverrideCombination = 'Cardboard Model'
structureDisplayModes = ['EntireStructure', 'CoreOnly', 'WithoutFinishes', 'StructureOnly']

navigatorItemIdsWithViewSettings = []
for i, oldViewSettings in enumerate(viewSettings):
    if 'error' in oldViewSettings:
        continue
    newViewSettings = {}
    if 'modelViewOptions' in oldViewSettings:
        newViewSettings['modelViewOptions'] = modelViewOptions[i % len(modelViewOptions)]['name']
    if 'layerCombination' in oldViewSettings:
        newViewSettings['layerCombination'] = layerCombinations[i % len(layerCombinations)]['name']
    if 'dimensionStyle' in oldViewSettings:
        newViewSettings['dimensionStyle'] = dimensionStyle
    if 'penSetName' in oldViewSettings:
        newViewSettings['penSetName'] = penSetName
    if 'graphicOverrideCombination' in oldViewSettings:
        newViewSettings['graphicOverrideCombination'] = graphicalOverrideCombination
    if 'structureDisplay' in oldViewSettings:
        newViewSettings['structureDisplay'] = structureDisplayModes[i % len(structureDisplayModes)]
    if 'rotation' in oldViewSettings:
        newViewSettings['rotation'] = oldViewSettings['rotation'] + math.radians(i * 5)
    if newViewSettings:
        navigatorItemIdsWithViewSettings.append({
            'navigatorItemId': views[i]['navigatorItemId'],
            'viewSettings': newViewSettings
        })

aclib.RunTapirCommand('SetViewSettings', {'navigatorItemIdsWithViewSettings': navigatorItemIdsWithViewSettings})

aclib.RunTapirCommand('GetViewSettings', {'navigatorItemIds': views})
