"""Locks a wall (manual lock, not Teamwork reservation) with LockElements, then unlocks it again.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('LockElements', {'elements': walls}))
print ('Editable while locked: {}'.format (len (aclib.RunTapirCommand ('FilterElements', {'elements': walls, 'filters': ['IsEditable']})['elements']) > 0))

aclib.RunTapirCommand ('UnlockElements', {'elements': walls})
