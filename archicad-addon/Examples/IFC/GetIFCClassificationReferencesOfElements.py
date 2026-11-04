"""Gets the IFC classification references of the first three walls with GetIFCClassificationReferencesOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetIFCClassificationReferencesOfElements', {
    'elements': walls,
    'storedOnly': False
})
