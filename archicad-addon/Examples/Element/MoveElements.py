"""Moves a wall by 1 m along the X axis with MoveElements, then moves it back.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [{'elementId': wallId, 'moveVector': {'x': 1.0, 'y': 0.0, 'z': 0.0}}]
}))

aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [{'elementId': wallId, 'moveVector': {'x': -1.0, 'y': 0.0, 'z': 0.0}}]
})
