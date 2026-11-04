"""Reads the built-in property values of the first Building Material, with their status, with GetPropertyValuesOfAttributes."""
import aclib

buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'}, debug = False)['attributes'][0]
attributeIds = [{'attributeId': buildingMaterial['attributeId']}]
propertyIds = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': attributeIds,
    'filter': 'FundamentalBuiltIn'
}, debug = False)['propertyDefinitionsOfAttributes'][0]['propertyIds']

aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
    'attributeIds': attributeIds,
    'properties': propertyIds,
    'includeStatus': True
})
