"""Prints the parent element, cut elements, geometry and extrusion settings of every opening with GetOpeningDetails.

Needs at least one Opening (made with the Opening tool). Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements']
if not openings:
    raise SystemExit ('No opening in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetOpeningDetails', {'elements': openings}))
