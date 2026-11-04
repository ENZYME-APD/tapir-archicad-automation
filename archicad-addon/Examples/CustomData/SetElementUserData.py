"""Stores a key-value pair on the first wall with SetElementUserData, reads it back, then removes it.

The data is saved with the project, invisible in the Archicad UI. Needs at least one Wall.
"""
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
wallId = walls[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('SetElementUserData', {
    'elementUserData': [
        {'elementId': wallId, 'userData': [{'key': 'tapir-example.externalId', 'value': 'W-001'}]}
    ]
}))
aclib.RunTapirCommand ('GetElementUserData', {'elements': walls})

aclib.PrintJson (aclib.RunTapirCommand ('SetElementUserData', {
    'elementUserData': [
        {'elementId': wallId, 'keysToDelete': ['tapir-example.externalId']}
    ]
}))
