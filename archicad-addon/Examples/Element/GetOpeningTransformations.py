"""Prints the placement (transformation matrices, origin and axes) of every window with GetOpeningTransformations.

Needs at least one Window; Doors, Skylights and Openings work the same way (Openings from Archicad 29).
Requires Archicad 27 or later.
"""
import aclib

windows = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Window'})['elements']
if not windows:
    raise SystemExit ('No window in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetOpeningTransformations', {'elements': windows}))
