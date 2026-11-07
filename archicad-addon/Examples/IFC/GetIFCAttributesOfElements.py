"""Gets the IFC attributes (Name, Description, ObjectType, Tag, ...) of the first three walls with GetIFCAttributesOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCAttributesOfElements', {
    'elements': walls,
    'storedOnly': False
}))
