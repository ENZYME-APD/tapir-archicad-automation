"""Raises the camera of the 3D perspective by 1 m, or turns the axonometry to isometric, with Set3DProjectionSettings.

Restores the original projection at the end.
"""
import aclib

settings = aclib.RunTapirCommand ('Get3DProjectionSettings')

if settings['isPersp']:
    # Every perspective parameter is required: pass back what was read, with the camera moved.
    perspective = settings['perspective']
    perspective['cameraPosition']['z'] += 1.0
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': perspective}))
    perspective['cameraPosition']['z'] -= 1.0
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': perspective}))
else:
    # An axonometry changes only the given values.
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'axonometry': {'projectionType': 'Isometric'}}))
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'axonometry': settings['axonometric']}))
