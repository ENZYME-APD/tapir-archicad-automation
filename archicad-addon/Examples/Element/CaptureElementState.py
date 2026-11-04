"""Captures the current state of the project's elements with CaptureElementState.

GetElementChangesSince can compare a later state to it. The script releases the state at the end.
"""
import aclib

state = aclib.RunTapirCommand ('CaptureElementState', {'type': 'ModificationStamp'})
print ('Captured state: {}'.format (state['stateId']['guid']))

aclib.RunTapirCommand ('ReleaseElementState', {'stateId': state['stateId']})
