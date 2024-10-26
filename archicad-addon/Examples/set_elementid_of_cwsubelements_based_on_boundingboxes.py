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

subelementTypeToElementIdPrefixAndStartingID = {
    'cWallPanels':      ['CWPanel',     1],
    'cWallFrames':      ['CWFrame',     1],
    'cWallJunctions':   ['CWJunction',  1],
    'cWallAccessories': ['CWAccessory', 1]
}

for subelementsOfCurrCW in allCWSubelements:
    for subelementType in subelementsOfCurrCW.keys():
        if subelementType not in subelementTypeToElementIdPrefixAndStartingID:
            continue
        elementIdPrefixAndStartingID = subelementTypeToElementIdPrefixAndStartingID[subelementType]

        subelements = subelementsOfCurrCW[subelementType]
        boundingBoxes = aclib.RunTapirCommand (
            'Get3DBoundingBoxes', {
                'elements' : subelements
            })['boundingBoxes3D']
        subelementsPairedWithBoundingBoxes = [(subelements[i],boundingBoxes[i]) for i in range(len(subelements))]
        subelementsPairedWithBoundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['xMin'])
        subelementsPairedWithBoundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['yMin'])
        subelementsPairedWithBoundingBoxes.sort (key=lambda pair:pair[1]['boundingBox3D']['zMin'])

        aclib.RunTapirCommand (
            'SetPropertyValuesOfElements', {
                'elementPropertyValues' : [{
                    'elementId': subelementsPairedWithBoundingBoxes[i][0]['elementId'],
                    'propertyId': elementIdPropertyId,
                    'propertyValue': {'value': '{}-{:03d}'.format (elementIdPrefixAndStartingID[0], i + elementIdPrefixAndStartingID[1])}
                } for i in range(len(subelementsPairedWithBoundingBoxes))]
            })
        elementIdPrefixAndStartingID[1] += len(subelementsPairedWithBoundingBoxes)