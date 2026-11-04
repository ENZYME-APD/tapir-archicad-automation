"""Reads the Structural Function and Position of the first walls with GetCategoryValuesOfElements.

Needs at least one Wall. Without categories every category of the elements is returned.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetCategoryValuesOfElements', {
    'elements': walls,
    'categories': ['StructuralFunction', 'Position']
}))
