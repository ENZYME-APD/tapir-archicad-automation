"""Lists what changed since a captured state with GetElementChangesSince.

Captures a state, creates a wall, reads the changes (the new wall), then deletes the wall and releases the state.
"""
import aclib

stateId = aclib.RunTapirCommand ('CaptureElementState', {})['stateId']

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 5.0, 'y': 0.0}, 'height': 3.0}]
})['elements']

changes = aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': stateId})
print ('{} new, {} modified, {} deleted elements.'.format (
    len (changes['newElements']), len (changes['modifiedElements']), len (changes['deletedElements'])))

aclib.RunTapirCommand ('DeleteElements', {'elements': walls})
aclib.RunTapirCommand ('ReleaseElementState', {'stateId': stateId})
