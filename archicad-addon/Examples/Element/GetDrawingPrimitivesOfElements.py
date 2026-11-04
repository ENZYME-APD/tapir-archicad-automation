"""Reads the 2D drawing primitives (lines, arcs, polygons ...) Archicad draws a wall from, with GetDrawingPrimitivesOfElements.

Needs at least one Wall. The floor plan must be the current window.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDrawingPrimitivesOfElements', {'elements': walls}))
