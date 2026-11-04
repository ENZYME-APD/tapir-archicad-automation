"""Reserves the first wall of the project with ReserveElements and prints the users who hold it if that fails.

Needs an open Teamwork project with at least one Wall. A wall that was free before is released again at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

wasFree = aclib.RunTapirCommand ('GetReservationStatusOfElements', {'elements': walls})['reservationStatuses'][0].get ('status') == 'Free'

response = aclib.RunTapirCommand ('ReserveElements', {'elements': walls})
for conflict in response.get ('conflicts', []):
    print ('Reserved by', conflict['user']['userName'])

if wasFree:
    aclib.RunTapirCommand ('ReleaseElements', {'elements': walls})
