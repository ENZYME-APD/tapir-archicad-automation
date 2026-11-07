"""Gets which roofs and shells trim the first three walls with GetElementTrims, and what each wall trims.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetElementTrims', {'elements': walls}))
