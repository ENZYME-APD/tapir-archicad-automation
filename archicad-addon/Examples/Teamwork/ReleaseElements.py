"""Reserves the first wall of the project and releases it again with ReleaseElements.

Needs an open Teamwork project with at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('ReserveElements', {'elements': walls})

aclib.PrintJson (aclib.RunTapirCommand ('ReleaseElements', {'elements': walls}))

aclib.RunTapirCommand ('GetReservationStatusOfElements', {'elements': walls})
