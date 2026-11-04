"""Removes a link created by LinkElements with UnlinkElements.

Links the first wall to the second one first, so it needs at least two Walls. Only the link in the given direction is removed.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Fewer than two walls in the project.')
link = {'fromElementId': walls[0]['elementId'], 'toElementId': walls[1]['elementId']}

aclib.RunTapirCommand ('LinkElements', {'links': [link]})

aclib.PrintJson (aclib.RunTapirCommand ('UnlinkElements', {'links': [{'fromElementId': walls[0]['elementId'], 'toElementId': walls[1]['elementId']}]}))
aclib.RunTapirCommand ('GetLinksOfElements', {'elements': walls[:1]})
