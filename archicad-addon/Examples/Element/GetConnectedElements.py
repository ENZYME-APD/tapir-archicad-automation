"""Lists the windows placed in the first walls with GetConnectedElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetConnectedElements', {
    'elements': walls,
    'connectedElementType': 'Window'
}))
