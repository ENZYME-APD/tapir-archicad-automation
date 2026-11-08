"""Scales a slab by 2 around the centre of its bounding box with ScaleElements, prints its bounding box before and after, then scales it back by 0.5.

Needs at least one editable Slab. The bounding box after scaling should be twice as wide and deep, with the same centre.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab', 'filters': ['IsEditable']})['elements'][:1]
if not slabs:
    raise SystemExit ('No editable slab in the project.')

def GetBox ():
    return aclib.RunTapirCommand ('Get2DBoundingBoxes', {'elements': slabs})['boundingBoxes2D'][0]['boundingBox2D']

box = GetBox ()
aclib.PrintJson (box)
center = {'x': (box['xMin'] + box['xMax']) / 2.0, 'y': (box['yMin'] + box['yMax']) / 2.0}

aclib.PrintJson (aclib.RunTapirCommand ('ScaleElements', {'elements': slabs, 'center': center, 'factor': 2.0}))
aclib.PrintJson (GetBox ())

aclib.RunTapirCommand ('ScaleElements', {'elements': slabs, 'center': center, 'factor': 0.5})
