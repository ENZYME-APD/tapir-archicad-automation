"""Unlocks a locked wall (manual lock, not Teamwork reservation) with UnlockElements.

Locks the first editable Wall first, so it is unchanged at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

aclib.RunTapirCommand ('LockElements', {'elements': walls})

aclib.PrintJson (aclib.RunTapirCommand ('UnlockElements', {'elements': walls}))
print ('Editable after unlocking: {}'.format (len (aclib.RunTapirCommand ('FilterElements', {'elements': walls, 'filters': ['IsEditable']})['elements']) > 0))
