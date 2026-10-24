import aclib

buildingMaterialAttributes = aclib.RunTapirCommand (
    'GetAttributesByType', {
        'attributeType': 'BuildingMaterial'
    })['attributes']
buildingMaterialAttributeIds = [{'attributeId': bma['attributeId']} for bma in buildingMaterialAttributes]

for propertyFilter in ['UserDefined', 'FundamentalBuiltIn', 'All']:
    aclib.RunTapirCommand (
        'GetPropertyDefinitionsOfAttributes', {
            'attributeIds': buildingMaterialAttributeIds,
            'filter': propertyFilter
        })['propertyDefinitionsOfAttributes']

customPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'UserDefined', 'localizedName': ['Tapir Group', 'Tapir Custom']}]})['properties'][0]['propertyId']

propertyValuesForBuildingMaterials = aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyId}]
    })['propertyValuesForAttributes']

attributeIdsWithCustomProperty = []
for i in range (len (propertyValuesForBuildingMaterials)):
    if 'error' not in propertyValuesForBuildingMaterials[i]['propertyValues'][0]:
        attributeIdsWithCustomProperty.append (buildingMaterialAttributeIds[i])

aclib.RunTapirCommand (
    'SetPropertyValueOfAttributes', {
        'propertyId': customPropertyId,
        'propertyValue': {'value': 'Audited'},
        'attributeIds': attributeIdsWithCustomProperty
    })

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': attributeIdsWithCustomProperty,
        'properties': [{'propertyId': customPropertyId}]
    })
