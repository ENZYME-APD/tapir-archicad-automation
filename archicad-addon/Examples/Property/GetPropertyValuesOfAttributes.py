"""Reads the built-in property values of the first Building Material, with their status, with GetPropertyValuesOfAttributes."""
import aclib

buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]
attributeIds = [{'attributeId': buildingMaterial['attributeId']}]
propertyIds = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': attributeIds,
    'filter': 'FundamentalBuiltIn'
})['propertyDefinitionsOfAttributes'][0]['propertyIds']

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
    'attributeIds': attributeIds,
    'properties': propertyIds,
    'includeStatus': True
}))
