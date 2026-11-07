"""Prints the GDL parameters of the cut and uncut floor plan symbols of every opening with GetOpeningSymbolParameters.

Needs at least one Opening (made with the Opening tool). Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements']
if not openings:
    raise SystemExit ('No opening in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetOpeningSymbolParameters', {'elements': openings}))
