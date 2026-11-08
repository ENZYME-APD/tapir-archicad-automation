"""Sets the Element ID of the first Wall with SetPropertyValuesOfElements, reads it back, then restores it.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

def ReadElementId ():
    return aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls,
        'properties': [{'propertyId': elementIdPropertyId}]
    })['propertyValuesForElements'][0]['propertyValues'][0]['propertyValue']['value']

oldValue = ReadElementId ()

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfElements', {
    'elementPropertyValues': [{
        'elementId': walls[0]['elementId'],
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': 'TAPIR-EXAMPLE'}
    }]
}))
print ('Element ID after the change:', ReadElementId ())

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfElements', {
    'elementPropertyValues': [{
        'elementId': walls[0]['elementId'],
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': oldValue}
    }]
}))
