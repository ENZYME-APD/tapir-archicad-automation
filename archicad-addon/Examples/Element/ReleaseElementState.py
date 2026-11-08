"""Frees a state captured by CaptureElementState with ReleaseElementState.

States are freed anyway when the project is closed; releasing them earlier frees their temporary file.
"""
import aclib

stateId = aclib.RunTapirCommand ('CaptureElementState', {})['stateId']

aclib.PrintJson (aclib.RunTapirCommand ('ReleaseElementState', {'stateId': stateId}))
