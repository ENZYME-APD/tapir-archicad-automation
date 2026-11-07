"""Aligns the left edges of three walls with AlignElements, prints their bounding boxes, then moves them back.

Needs at least two editable Walls on the floor plan.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:3]
if len (walls) < 2:
    raise SystemExit ('Fewer than two editable walls in the project.')

def GetBoxes ():
    return aclib.RunTapirCommand ('Get2DBoundingBoxes', {'elements': walls})['boundingBoxes2D']

before = GetBoxes ()

aclib.PrintJson (aclib.RunTapirCommand ('AlignElements', {'elements': walls, 'alignTo': 'left'}))

after = GetBoxes ()
aclib.PrintJson ([box['boundingBox2D']['xMin'] for box in after])

aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [
        {'elementId': wall['elementId'],
         'moveVector': {'x': old['boundingBox2D']['xMin'] - new['boundingBox2D']['xMin'], 'y': 0.0, 'z': 0.0}}
        for wall, old, new in zip (walls, before, after)
    ]
})
