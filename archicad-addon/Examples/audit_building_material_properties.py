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
