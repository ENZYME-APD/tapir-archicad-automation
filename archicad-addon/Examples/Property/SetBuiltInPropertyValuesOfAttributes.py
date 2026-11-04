"""Sets an editable built-in text property of the first Building Material with SetBuiltInPropertyValuesOfAttributes, then restores it."""
import aclib

buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]
attributeIds = [{'attributeId': buildingMaterial['attributeId']}]
propertyIds = aclib.RunTapirCommand ('GetPropertyDefinitionsOfAttributes', {
    'attributeIds': attributeIds,
    'filter': 'FundamentalBuiltIn'
})['propertyDefinitionsOfAttributes'][0]['propertyIds']
textProperty = next ((p for p in aclib.RunTapirCommand ('GetAllProperties', {'properties': propertyIds, 'includeAvailability': False})['properties']
                      if p['propertyValueType'] == 'String' and p['propertyCollectionType'] == 'Single' and p['propertyIsEditable']), None)
if textProperty is None:
    raise SystemExit ('The Building Material has no editable built-in text property.')
propertyId = textProperty['propertyId']
print ('Property:', textProperty['propertyGroupName'], '/', textProperty['propertyName'])

def ReadValue ():
    return aclib.RunTapirCommand ('GetPropertyValuesOfAttributes', {
        'attributeIds': attributeIds,
        'properties': [{'propertyId': propertyId}]
    })['propertyValuesForAttributes'][0]['propertyValues'][0]['propertyValue']['value']

oldValue = ReadValue ()

aclib.PrintJson (aclib.RunTapirCommand ('SetBuiltInPropertyValuesOfAttributes', {
    'attributes': [{
        'attributeId': buildingMaterial['attributeId'],
        'properties': [{'propertyId': propertyId, 'propertyValue': {'value': 'Set by Tapir'}}]
    }]
}))
print ('Value after the change:', ReadValue ())

aclib.PrintJson (aclib.RunTapirCommand ('SetBuiltInPropertyValuesOfAttributes', {
    'attributes': [{
        'attributeId': buildingMaterial['attributeId'],
        'properties': [{'propertyId': propertyId, 'propertyValue': {'value': oldValue}}]
    }]
}))
