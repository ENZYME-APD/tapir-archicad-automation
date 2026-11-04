"""Hands the reservation of the first wall over to another team member with GrantElements.

Needs an open Teamwork project with at least one Wall and another member. The wall is reserved first, and stays
reserved by the other member. Requires Archicad 27 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

myUserId = aclib.RunTapirCommand ('GetTeamworkProjectInfo', debug = False).get ('userId')
users = aclib.RunTapirCommand ('GetTeamworkStatus', debug = False).get ('users', [])
otherUsers = [user for user in users if user['userId'] != myUserId]
if not otherUsers:
    raise SystemExit ('The project has no other Teamwork member.')

aclib.RunTapirCommand ('ReserveElements', {'elements': walls}, debug = False)

aclib.RunTapirCommand ('GrantElements', {'elements': walls, 'userId': otherUsers[0]['userId']})
