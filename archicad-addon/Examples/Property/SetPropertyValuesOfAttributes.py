"""Sets an editable custom text property of a Building Material with SetPropertyValuesOfAttributes, reads it back, then restores it.

Needs a Building Material that has an editable, not expression based, custom text property.
"""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes']
attributeIds = [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials]
definitions = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {'attributeIds': attributeIds})['propertyDefinitionsOfAttributes']
textPropertyIds = [p['propertyId'] for p in aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['properties']
                   if p['propertyValueType'] == 'String' and p['propertyCollectionType'] == 'Single' and p['propertyIsEditable'] and not p['isExpressionBased']]
target = next (((attributeId, p['propertyId']) for attributeId, d in zip (attributeIds, definitions) for p in d.get ('propertyIds', []) if p['propertyId'] in textPropertyIds), None)
if target is None:
    raise SystemExit ('No Building Material has an editable custom text property.')
attributeId, propertyId = target

def ReadValue ():
    return aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
        'attributeIds': [attributeId],
        'properties': [{'propertyId': propertyId}]
    })['propertyValuesForAttributes'][0]['propertyValues'][0]['propertyValue']['value']

oldValue = ReadValue ()

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfAttributes', {
    'attributePropertyValues': [{'attributeId': attributeId['attributeId'], 'propertyId': propertyId, 'propertyValue': {'value': 'Set by Tapir'}}]
}))
print ('Value after the change:', ReadValue ())

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfAttributes', {
    'attributePropertyValues': [{'attributeId': attributeId['attributeId'], 'propertyId': propertyId, 'propertyValue': {'value': oldValue}}]
}))
