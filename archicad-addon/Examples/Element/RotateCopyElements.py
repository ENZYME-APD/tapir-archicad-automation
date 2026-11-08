"""Makes a circular array of 3 copies of a wall around the project origin, 90 degrees apart, with RotateCopyElements, then deletes the copies.

Needs at least one editable Wall. The angle is in radians, counterclockwise; copy n is rotated by
n times the angle.
"""
import math
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')

response = aclib.RunTapirCommand ('RotateCopyElements', {
    'elements': walls,
    'center': {'x': 0.0, 'y': 0.0},
    'angle': math.pi / 2,
    'count': 3
})

for copy in response['copies']:
    print ('Copies made:', len (copy['elements']), copy.get ('error', ''))

newElements = [element for copy in response['copies'] for element in copy['elements']]
aclib.RunTapirCommand ('DeleteElements', {'elements': newElements})
