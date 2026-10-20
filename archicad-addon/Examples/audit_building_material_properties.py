import aclib

buildingMaterialAttributes = aclib.RunTapirCommand (
    'GetAttributesByType', {
        'attributeType': 'BuildingMaterial'
    })['attributes']
buildingMaterialAttributeIds = [{'attributeId': bma['attributeId']} for bma in buildingMaterialAttributes]

propertyDefinitionsByFilter = {}
for propertyFilter in ['UserDefined', 'FundamentalBuiltIn', 'All']:
    propertyDefinitionsByFilter[propertyFilter] = aclib.RunTapirCommand (
        'GetPropertyDefinitionsOfAttributes', {
            'attributeIds': buildingMaterialAttributeIds,
            'filter': propertyFilter
        })['propertyDefinitionsOfAttributes']

userDefinedPropertyIds = []
for propertyDefinitionsOfAttribute in propertyDefinitionsByFilter['UserDefined']:
    for propertyId in propertyDefinitionsOfAttribute.get ('propertyIds', []):
        if propertyId not in userDefinedPropertyIds:
            userDefinedPropertyIds.append (propertyId)

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': userDefinedPropertyIds,
        'includeStatus': True
    })
