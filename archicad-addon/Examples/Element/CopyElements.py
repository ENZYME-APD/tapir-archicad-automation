"""Makes a linear array of 5 copies of a wall, 2 m apart along the X axis, with CopyElements, then deletes the copies.

Needs at least one editable Wall. Copy n is moved by n times the offset; the response lists the
new elements of each given element in the order of the given ones.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')

response = aclib.RunTapirCommand ('CopyElements', {
    'elements': walls,
    'offset': {'x': 2.0, 'y': 0.0, 'z': 0.0},
    'count': 5
})

for copy in response['copies']:
    print ('Copies made:', len (copy['elements']), copy.get ('error', ''))

newElements = [element for copy in response['copies'] for element in copy['elements']]
aclib.RunTapirCommand ('DeleteElements', {'elements': newElements})
