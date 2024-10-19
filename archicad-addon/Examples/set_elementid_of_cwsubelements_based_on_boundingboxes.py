import aclib

allCurtainWalls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']

allCWSubelements = aclib.RunTapirCommand (
    'GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements': allCurtainWalls
    })['subelementsOfHierarchicalElements']

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [{"type": "BuiltIn", "nonLocalizedName": "General_ElementID"}]})['properties'][0]['propertyId']

for subelementsOfCurrCW in allCWSubelements:
    for subelementType in subelementsOfCurrCW.keys():
        subelements = subelementsOfCurrCW[subelementType]
        boundingBoxes = aclib.RunTapirCommand (
            'Get3DBoundingBoxes', {
                'elements' : subelements
            })['boundingBoxes3D']
        subelementsPairedWithboundingBoxes = [(subelements[i],boundingBoxes[i]) for i in range(len(subelements))]
        subelementsPairedWithboundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['xMin'])
        subelementsPairedWithboundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['yMin'])
        subelementsPairedWithboundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['zMin'])

        response = aclib.RunTapirCommand (
            'SetPropertyValuesOfElements', {
                'elementPropertyValues' : [{
                    'elementId': subelementsPairedWithboundingBoxes[i][0]['elementId'],
                    'propertyId': elementIdPropertyId,
                    'propertyValue': {'value': '{}-{:03d}'.format(subelementType, i+1)}
                } for i in range(len(subelementsPairedWithboundingBoxes))]
            })