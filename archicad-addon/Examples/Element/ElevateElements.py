"""Moves a wall up by 1 m with ElevateElements, like Edit > Move > Elevate, then moves it back down.

Needs at least one editable Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsEditable']})['elements'][:1]
if not walls:
    raise SystemExit ('No editable wall in the project.')
wallId = walls[0]['elementId']

aclib.PrintJson (aclib.RunTapirCommand ('ElevateElements', {
    'elementsWithElevations': [{'elementId': wallId, 'elevation': 1.0}]
}))

aclib.RunTapirCommand ('ElevateElements', {
    'elementsWithElevations': [{'elementId': wallId, 'elevation': -1.0}]
})
