"""Reads the 3D bounding boxes of the first walls with Get3DBoundingBoxes.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('Get3DBoundingBoxes', {'elements': walls})
