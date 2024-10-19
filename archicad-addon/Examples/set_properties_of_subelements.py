import aclib

allCurtainWalls = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']

allCWSubelements = aclib.RunTapirCommand (
    'GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements': allCurtainWalls
    })['subelementsOfHierarchicalElements']

allCWFrameSubelements = [subelement for subelements in allCWSubelements for subelement in subelements['cWallFrames']]

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [{"type": "BuiltIn", "nonLocalizedName": "General_ElementID"}]})['properties'][0]['propertyId']

response = aclib.RunTapirCommand (
    'GetPropertyValuesOfElements', {
        'elements' : [{
            'elementId': subelement['elementId']
        } for subelement in allCWFrameSubelements],
        'properties' : [{
            'propertyId': elementIdPropertyId
        }]
    })

elementPropertyValues = []

for i in range(len(allCWFrameSubelements)):
    previousElementIdOfCWallFrame = response['propertyValuesForElements'][i]['propertyValues'][0]['propertyValue']['value']
    newElementIdOfCWallFrame = 'NewID-{:04d} (PrevID: {})'.format(i, previousElementIdOfCWallFrame)

    elementPropertyValues.append({
        'elementId': allCWFrameSubelements[i]['elementId'],
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': newElementIdOfCWallFrame}
    })

response = aclib.RunTapirCommand (
    'SetPropertyValuesOfElements', {
        'elementPropertyValues' : elementPropertyValues
    })