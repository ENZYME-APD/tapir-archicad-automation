"""Prints the built-in properties of the first component of the first wall with GetPropertyDefinitionsOfElementComponents.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
componentsOfWall = aclib.RunTapirCommand ('GetComponentsOfElements', {'elements': walls})['componentsOfElements'][0]
if not componentsOfWall.get ('elementComponents'):
    raise SystemExit ('The wall has no components.')

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyDefinitionsOfElementComponents', {
    'elementComponents': componentsOfWall['elementComponents'][:1],
    'filter': 'BuiltIn'
}))
