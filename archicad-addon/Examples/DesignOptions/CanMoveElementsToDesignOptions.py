"""Checks which walls of the project can be moved to another design option, with CanMoveElementsToDesignOptions.

Requires Archicad 29 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CanMoveElementsToDesignOptions', {'elements': walls}))
