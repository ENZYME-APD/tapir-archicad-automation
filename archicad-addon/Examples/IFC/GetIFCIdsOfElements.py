"""Gets the IFC identifiers of the first three walls with GetIFCIdsOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetIFCIdsOfElements', {'elements': walls})
