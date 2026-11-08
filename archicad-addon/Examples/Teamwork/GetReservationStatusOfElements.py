"""Gets the Teamwork reservation status of the first three walls with GetReservationStatusOfElements, without reserving them.

Needs at least one Wall. In a solo project every element is editable.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetReservationStatusOfElements', {'elements': walls}))
