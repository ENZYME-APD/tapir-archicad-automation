"""Rotates a wall by 90 degrees around the project origin with RotateElements, then rotates it back.

Needs at least one editable Wall. The angle is the one between origin-beginPoint and origin-endPoint.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

origin = {'x': 0.0, 'y': 0.0}
xAxis = {'x': 1.0, 'y': 0.0}
yAxis = {'x': 0.0, 'y': 1.0}

aclib.PrintJson (aclib.RunTapirCommand ('RotateElements', {
    'elementsWithRotations': [{'elementId': wallId, 'rotation': {'beginPoint': xAxis, 'endPoint': yAxis, 'origin': origin}}]
}))

aclib.RunTapirCommand ('RotateElements', {
    'elementsWithRotations': [{'elementId': wallId, 'rotation': {'beginPoint': yAxis, 'endPoint': xAxis, 'origin': origin}}]
})
