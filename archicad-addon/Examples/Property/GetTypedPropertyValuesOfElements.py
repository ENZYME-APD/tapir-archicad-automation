"""Reads the Element ID and the thickness of the first three Walls with GetTypedPropertyValuesOfElements.

The values come in JSON's own types: the Element ID as a string, the thickness as a number in meters,
whatever the project's working units. Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
propertyIds = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [
        {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'},
        {'type': 'BuiltIn', 'nonLocalizedName': 'General_Thickness'}
    ]
})['properties']

aclib.PrintJson (aclib.RunTapirCommand ('GetTypedPropertyValuesOfElements', {
    'elements': walls,
    'properties': [{'propertyId': p['propertyId']} for p in propertyIds]
}))
