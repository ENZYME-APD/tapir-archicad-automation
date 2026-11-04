"""Tells whether the Element ID can be modified for the first Wall and in the Wall tool default with GetPropertyEditability.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyEditability', {
    'properties': [{'propertyId': elementIdPropertyId}],
    'elements': walls,
    'elementTypes': ['Wall']
}))
