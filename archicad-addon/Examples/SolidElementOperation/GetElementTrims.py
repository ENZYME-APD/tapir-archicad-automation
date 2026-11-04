"""Gets which roofs and shells trim the first three walls with GetElementTrims, and what each wall trims.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetElementTrims', {'elements': walls})
