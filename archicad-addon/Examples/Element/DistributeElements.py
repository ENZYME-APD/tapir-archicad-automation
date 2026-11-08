"""Distributes up to five walls along X with equal gaps using DistributeElements, prints their new left edges, then moves them back.

Needs at least three editable Walls on the floor plan.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:5]
if len (walls) < 3:
    raise SystemExit ('Fewer than three editable walls in the project.')

def GetBoxes ():
    return aclib.RunTapirCommand ('Get2DBoundingBoxes', {'elements': walls})['boundingBoxes2D']

before = GetBoxes ()

aclib.PrintJson (aclib.RunTapirCommand ('DistributeElements', {'elements': walls, 'direction': 'horizontal', 'mode': 'gaps'}))

after = GetBoxes ()
aclib.PrintJson ([box['boundingBox2D']['xMin'] for box in after])

aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [
        {'elementId': wall['elementId'],
         'moveVector': {'x': old['boundingBox2D']['xMin'] - new['boundingBox2D']['xMin'], 'y': 0.0, 'z': 0.0}}
        for wall, old, new in zip (walls, before, after)
    ]
})
