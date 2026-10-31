"""
Finds what changed in the project since a captured moment, instead of re-reading every element.

CaptureElementState stores a state of the project in the add-on and returns a stateId.
GetElementChangesSince lists the elements created, modified and deleted since that state
(or between two stored states). The states only last for the Archicad session: they are gone
when the project is closed or Archicad restarts, so capture a new one after that, and
release the ones you no longer need.
"""

import aclib

# The default type compares the modification stamps of the elements; '3DModel' would count
# only changes that alter the 3D model.
startState = aclib.RunTapirCommand ('CaptureElementState', {'type': 'ModificationStamp'})['stateId']

wall = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'begCoordinate': { 'x': 0.0, 'y': 0.0 },
            'endCoordinate': { 'x': 5.0, 'y': 0.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})
wallId = wall['elements'][0]['elementId']

# Since startState: the new wall.
aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': startState})

afterCreateState = aclib.RunTapirCommand ('CaptureElementState')['stateId']

aclib.RunTapirCommand ('MoveElements', {
    'elementsWithMoveVectors': [
        {
            'elementId': wallId,
            'moveVector': { 'x': 1.0, 'y': 1.0, 'z': 0.0 }
        }
    ]
})

# Since afterCreateState: the wall is modified. Since startState it is still new.
aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': afterCreateState})

aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': wallId}]})

# Since afterCreateState: the wall is deleted.
aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': afterCreateState})

# Two stored states can be compared too: between startState and afterCreateState the wall was created.
aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': startState, 'toStateId': afterCreateState})

aclib.RunTapirCommand ('ReleaseElementState', {'stateId': startState})
aclib.RunTapirCommand ('ReleaseElementState', {'stateId': afterCreateState})

# A released (or otherwise unknown) stateId is an error.
aclib.RunTapirCommand ('GetElementChangesSince', {'stateId': startState})
