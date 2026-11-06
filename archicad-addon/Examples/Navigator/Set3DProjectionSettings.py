"""Raises the camera of the 3D perspective by 1 m and puts its sun at 21 June 13:30, or turns the axonometry to isometric, with Set3DProjectionSettings.

The sun of a projection only changes the values given in sun; the others are kept.
Restores the original projection at the end.
"""
import copy
import aclib

settings = aclib.RunTapirCommand ('Get3DProjectionSettings')

if settings['isPersp']:
    # Every perspective parameter but sun is required: pass back what was read, with the camera moved.
    original = copy.deepcopy (settings['perspective'])
    perspective = settings['perspective']
    perspective['cameraPosition']['z'] += 1.0
    perspective['sun'] = {'sunPositionOption': 'DateAndTime', 'month': 6, 'day': 21, 'hour': 13, 'minute': 30, 'second': 0}
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': perspective}))
    aclib.PrintJson (aclib.RunTapirCommand ('Get3DProjectionSettings')['perspective']['sun'])
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': original}))
else:
    # An axonometry changes only the given values.
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'axonometry': {'projectionType': 'Isometric'}}))
    aclib.PrintJson (aclib.RunTapirCommand ('Set3DProjectionSettings', {'axonometry': settings['axonometric']}))
