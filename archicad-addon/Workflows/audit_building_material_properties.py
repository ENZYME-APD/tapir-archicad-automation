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

# AddPropertyToAttributes and RemovePropertyFromAttributes work in Archicad 25, 26 and 27 only.
customPropertyIdToAdd = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'UserDefined', 'localizedName': ['Tapir Group', 'Tapir Custom']}]})['properties'][0]['propertyId']

aclib.RunTapirCommand (
    'AddPropertyToAttributes', {
        'propertyId': customPropertyIdToAdd,
        'attributeIds': buildingMaterialAttributeIds
    })

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyIdToAdd}]
    })

# SetPropertyValueOfAttributes sets one value on many attributes in one undo step. It fails as a whole, changing
# nothing, when the property is not available for one of them, so only the Building Materials that have it are given.
propertyValuesForBuildingMaterials = aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyIdToAdd}]
    })['propertyValuesForAttributes']

attributeIdsWithCustomProperty = []
for i in range (len (propertyValuesForBuildingMaterials)):
    if 'error' not in propertyValuesForBuildingMaterials[i]['propertyValues'][0]:
        attributeIdsWithCustomProperty.append (buildingMaterialAttributeIds[i])

aclib.RunTapirCommand (
    'SetPropertyValueOfAttributes', {
        'propertyId': customPropertyIdToAdd,
        'propertyValue': {'value': 'Audited'},
        'attributeIds': attributeIdsWithCustomProperty
    })

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': attributeIdsWithCustomProperty,
        'properties': [{'propertyId': customPropertyIdToAdd}]
    })

aclib.RunTapirCommand (
    'RemovePropertyFromAttributes', {
        'propertyId': customPropertyIdToAdd,
        'attributeIds': buildingMaterialAttributeIds
    })

aclib.RunTapirCommand (
    'GetPropertyValuesOfAttributes', {
        'attributeIds': buildingMaterialAttributeIds,
        'properties': [{'propertyId': customPropertyIdToAdd}]
    })
