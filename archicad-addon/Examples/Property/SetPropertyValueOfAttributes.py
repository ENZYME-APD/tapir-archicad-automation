"""Sets one value of a custom text property on every Building Material that has it with SetPropertyValueOfAttributes, then restores the values.

Needs Building Materials that have an editable, not expression based, custom text property.
"""
import aclib

buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes']
attributeIds = [{'attributeId': buildingMaterial['attributeId']} for buildingMaterial in buildingMaterials]
definitions = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {'attributeIds': attributeIds})['propertyDefinitionsOfAttributes']
textPropertyIds = [p['propertyId'] for p in aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['properties']
                   if p['propertyValueType'] == 'String' and p['propertyCollectionType'] == 'Single' and p['propertyIsEditable'] and not p['isExpressionBased']]
propertyId = next ((p['propertyId'] for d in definitions for p in d.get ('propertyIds', []) if p['propertyId'] in textPropertyIds), None)
if propertyId is None:
    raise SystemExit ('No Building Material has an editable custom text property.')
attributeIds = [a for a, d in zip (attributeIds, definitions) if {'propertyId': propertyId} in d.get ('propertyIds', [])]

def ReadValues ():
    return [values['propertyValues'][0]['propertyValue']['value'] for values in aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
        'attributeIds': attributeIds,
        'properties': [{'propertyId': propertyId}]
    })['propertyValuesForAttributes']]

oldValues = ReadValues ()

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValueOfAttributes', {
    'propertyId': propertyId,
    'propertyValue': {'value': 'Set by Tapir'},
    'attributeIds': attributeIds
}))
print ('Values after the change:', ReadValues ())

aclib.RunTapirCommand ('SetPropertyValuesOfAttributes', {
    'attributePropertyValues': [{'attributeId': a['attributeId'], 'propertyId': propertyId, 'propertyValue': {'value': v}} for a, v in zip (attributeIds, oldValues)]
})
