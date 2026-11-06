"""Reads the 2D bounding boxes of the first walls, as Archicad draws them on the floor plan, with Get2DBoundingBoxes.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('Get2DBoundingBoxes', {'elements': walls}))
