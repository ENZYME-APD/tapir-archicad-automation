"""Lists the property objects (the library parts of the old listing system) linked to the first walls with GetPropertyObjectsOfElements.

Needs at least one Wall. Usually only projects migrated from older Archicad versions have property objects.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetPropertyObjectsOfElements', {'elements': walls})
