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
