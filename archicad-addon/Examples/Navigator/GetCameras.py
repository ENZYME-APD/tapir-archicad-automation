"""Prints the Camera Sets of the project with their cameras with GetCameras, then shows the first camera's view in the 3D window with Set3DProjectionSettings."""
import aclib

response = aclib.RunTapirCommand ('GetCameras')
aclib.PrintJson (response)

cameras = [camera for cameraSet in response['cameraSets'] for camera in cameraSet['cameras']]
if cameras:
    aclib.RunTapirCommand ('Set3DProjectionSettings', {'perspective': cameras[0]['perspective']})
