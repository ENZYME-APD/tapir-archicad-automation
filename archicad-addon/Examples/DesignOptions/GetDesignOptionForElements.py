"""Prints which design option each wall of the project belongs to, with GetDesignOptionForElements.

Requires Archicad 29 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDesignOptionForElements', {'elements': walls}))
