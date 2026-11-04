"""Reads how property objects are linked to the first walls and to the wall tool defaults with GetLinkedPropertyObjectsOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

# The last item reads the links of the wall tool defaults instead of an element.
aclib.PrintJson (aclib.RunTapirCommand ('GetLinkedPropertyObjectsOfElements', {
    'elements': walls + [{'elementType': 'Wall'}]
}))
