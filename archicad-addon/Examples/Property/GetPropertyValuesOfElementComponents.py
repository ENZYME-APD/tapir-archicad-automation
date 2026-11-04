"""Prints the building material name of each component of the first wall with GetPropertyValuesOfElementComponents.

Needs at least one Wall. The Building Material Name property has the same guid in every language.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
componentsOfWall = aclib.RunTapirCommand ('GetComponentsOfElements', {'elements': walls})['componentsOfElements'][0]
if not componentsOfWall.get ('elementComponents'):
    raise SystemExit ('The wall has no components.')

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyValuesOfElementComponents', {
    'elementComponents': componentsOfWall['elementComponents'],
    'properties': [{'propertyId': {'guid': '514C85CB-3FC8-4EDF-A8E9-44E13969CBE7'}}]
}))
