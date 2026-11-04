"""Reads the Element ID of the first three Walls with GetPropertyValuesOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
    'elements': walls,
    'properties': [{'propertyId': elementIdPropertyId}],
    'includeAvailability': True
}))
