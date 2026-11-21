"""Creates an empty Camera Set (camera path) named "Viewpoints" with CreateCameraSets, makes it the active set, and lists the Camera Sets with GetCameras."""
import aclib

response = aclib.RunTapirCommand ('CreateCameraSets', {
    'cameraSets': [
        {'name': 'Viewpoints', 'isActive': True}
    ]
})
aclib.PrintJson (response)

aclib.PrintJson (aclib.RunTapirCommand ('GetCameras'))
