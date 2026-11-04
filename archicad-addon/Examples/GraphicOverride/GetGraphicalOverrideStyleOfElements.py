"""Gets the graphic override style the first three walls are drawn with in the current window with GetGraphicalOverrideStyleOfElements.

Needs at least one Wall. Requires Archicad 27 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetGraphicalOverrideStyleOfElements', {'elements': walls}))
