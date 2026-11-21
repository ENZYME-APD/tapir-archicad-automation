"""Creates a Camera Set with a camera using CreateCameraSets and CreateCameras, then moves the camera and its target, makes it a two-point perspective with a new sun and pen using ModifyCameras; the camera keeps its element id. Lists the cameras with GetCameras."""
import aclib

cameraSet = aclib.RunTapirCommand ('CreateCameraSets', {
    'cameraSets': [
        {'name': 'Viewpoints to modify'}
    ]
})['elements'][0]['elementId']

camera = aclib.RunTapirCommand ('CreateCameras', {
    'cameras': [
        {
            'cameraSetElementId': cameraSet,
            'cameraPosition': {'x': -10.0, 'y': -10.0, 'z': 1.7},
            'targetPosition': {'x': 0.0, 'y': 0.0, 'z': 1.5}
        }
    ]
})['elements'][0]['elementId']

response = aclib.RunTapirCommand ('ModifyCameras', {
    'cameras': [
        {
            'cameraElementId': camera,
            'cameraPosition': {'x': 10.0, 'y': -10.0, 'z': 2.0},
            'targetPosition': {'x': 0.0, 'y': 5.0, 'z': 1.5},
            'isTwoPointPersp': True,
            'sun': {'sunPositionOption': 'DateAndTime', 'year': 2026, 'month': 12, 'day': 21, 'hour': 10, 'minute': 0, 'second': 0},
            'pen': 3
        }
    ]
})
aclib.PrintJson (response)

aclib.PrintJson (aclib.RunTapirCommand ('GetCameras'))
