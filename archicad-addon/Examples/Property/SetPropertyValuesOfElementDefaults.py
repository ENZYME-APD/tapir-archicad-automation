"""Sets the Element ID of the Wall tool default with SetPropertyValuesOfElementDefaults, reads it back, then restores it.

New walls get the tool default's values.
"""
import aclib

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

def ReadDefaultElementId ():
    return aclib.RunTapirCommand ('GetPropertyValuesOfElementDefaults', {
        'elementType': 'Wall',
        'properties': [{'propertyId': elementIdPropertyId}]
    })['propertyValues'][0]['propertyValue']['value']

oldValue = ReadDefaultElementId ()

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfElementDefaults', {
    'elementDefaultPropertyValues': [{
        'elementType': 'Wall',
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': 'TAPIR-WALL'}
    }]
}))
print ('Default Element ID after the change:', ReadDefaultElementId ())

aclib.PrintJson (aclib.RunTapirCommand ('SetPropertyValuesOfElementDefaults', {
    'elementDefaultPropertyValues': [{
        'elementType': 'Wall',
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': oldValue}
    }]
}))
