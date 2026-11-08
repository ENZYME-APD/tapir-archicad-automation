"""Gets the solid element operation links of the first three walls with GetSolidElementLinks, as target and as operator.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetSolidElementLinks', {'elements': walls}))
