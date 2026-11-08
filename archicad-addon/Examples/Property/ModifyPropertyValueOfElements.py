"""Sets the same Element ID on the first three Walls in one step with ModifyPropertyValueOfElements, then restores their values.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

def ReadElementIds ():
    return [values['propertyValues'][0]['propertyValue']['value'] for values in aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls,
        'properties': [{'propertyId': elementIdPropertyId}]
    })['propertyValuesForElements']]

oldValues = ReadElementIds ()

aclib.PrintJson (aclib.RunTapirCommand ('ModifyPropertyValueOfElements', {
    'elements': walls,
    'propertyId': elementIdPropertyId,
    'propertyValue': {'value': 'TAPIR-EXAMPLE'}
}))
print ('Element IDs after the change:', ReadElementIds ())

aclib.RunTapirCommand ('SetPropertyValuesOfElements', {
    'elementPropertyValues': [{'elementId': wall['elementId'], 'propertyId': elementIdPropertyId, 'propertyValue': {'value': value}} for wall, value in zip (walls, oldValues)]
})
