"""Gets the IFC classification references of the first three walls with GetIFCClassificationReferencesOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
    'elements': walls,
    'storedOnly': False
}))
