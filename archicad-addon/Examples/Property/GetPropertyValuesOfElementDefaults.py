"""Reads the Element ID of the Wall tool default, which new walls get, with GetPropertyValuesOfElementDefaults."""
import aclib

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyValuesOfElementDefaults', {
    'elementType': 'Wall',
    'properties': [{'propertyId': elementIdPropertyId}]
}))
