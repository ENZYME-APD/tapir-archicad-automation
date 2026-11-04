"""Makes a custom property available for the first Building Material with AddPropertyToAttributes, then removes it again.

Needs a custom property that the Building Material does not have yet. Requires Archicad 25, 26 or 27.
"""
import aclib

buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]
attributeIds = [{'attributeId': buildingMaterial['attributeId']}]
ownPropertyIds = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {'attributeIds': attributeIds})['propertyDefinitionsOfAttributes'][0].get ('propertyIds', [])
customProperties = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['properties']
propertyId = next ((p['propertyId'] for p in customProperties if {'propertyId': p['propertyId']} not in ownPropertyIds), None)
if propertyId is None:
    raise SystemExit ('No custom property that the Building Material does not have.')

aclib.PrintJson (aclib.RunTapirCommand ('AddPropertyToAttributes', {'propertyId': propertyId, 'attributeIds': attributeIds}))
aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {'attributeIds': attributeIds, 'properties': [{'propertyId': propertyId}]})

aclib.RunTapirCommand ('RemovePropertyFromAttributes', {'propertyId': propertyId, 'attributeIds': attributeIds})
