"""Creates a Camera Set named "Viewpoints" with CreateCameraSets and adds two perspective cameras to its path with CreateCameras, the second one a two-point perspective with the sun given by date and time; then lists the cameras with GetCameras."""
import aclib

cameraSet = aclib.RunTapirCommand ('CreateCameraSets', {
    'cameraSets': [
        {'name': 'Viewpoints', 'isActive': True}
    ]
})['elements'][0]['elementId']

response = aclib.RunTapirCommand ('CreateCameras', {
    'cameras': [
        {
            'cameraSetElementId': cameraSet,
            'cameraPosition': {'x': -10.0, 'y': -10.0, 'z': 1.7},
            'targetPosition': {'x': 0.0, 'y': 0.0, 'z': 1.5}
        },
        {
            'cameraSetElementId': cameraSet,
            'cameraPosition': {'x': 10.0, 'y': -10.0, 'z': 1.7},
            'targetPosition': {'x': 0.0, 'y': 0.0, 'z': 1.5},
            'isTwoPointPersp': True,
            'sun': {'sunPositionOption': 'DateAndTime', 'year': 2026, 'month': 6, 'day': 21, 'hour': 14, 'minute': 0, 'second': 0},
            'pen': 5
        }
    ]
})
aclib.PrintJson (response)

aclib.PrintJson (aclib.RunTapirCommand ('GetCameras'))
