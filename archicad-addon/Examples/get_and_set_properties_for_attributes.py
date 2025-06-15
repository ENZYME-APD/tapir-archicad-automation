import aclib

customPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'UserDefined', 'localizedName': ['Tapir Group', 'Tapir Custom']}]})['properties'][0]['propertyId']

buildingMaterialAttributes = aclib.RunTapirCommand (
    'GetAttributesByType', {
        'attributeType': 'BuildingMaterial'
    })['attributes']
buildingMaterialAttributeIds = [{'attributeId': bma['attributeId']} for bma in buildingMaterialAttributes]

propertyValuesForBuildingMaterials = aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyId}]
    })['propertyValuesForAttributes']

attributePropertyValues = []
for i in range(len(propertyValuesForBuildingMaterials)):
    propertyValue = propertyValuesForBuildingMaterials[i]['propertyValues'][0]
    if 'error' not in propertyValue:
        prevValue = propertyValue['propertyValue']['value']
        newValue = 'New {}'.format (prevValue)

        attributePropertyValues.append ({
                'attributeId': buildingMaterialAttributeIds[i]['attributeId'],
                'propertyId': customPropertyId,
                'propertyValue': {'value': newValue}
            })

aclib.RunTapirCommand (
    'SetPropertyValuesOfAttributes', {
        'attributePropertyValues' : attributePropertyValues
    })

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyId}]
    })['propertyValuesForAttributes']