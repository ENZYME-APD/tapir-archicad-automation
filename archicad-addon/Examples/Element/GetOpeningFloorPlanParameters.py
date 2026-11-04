"""Prints the floor plan parameters of every opening with GetOpeningFloorPlanParameters.

Needs at least one Opening (made with the Opening tool). Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements']
if not openings:
    raise SystemExit ('No opening in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetOpeningFloorPlanParameters', {'elements': openings}))
