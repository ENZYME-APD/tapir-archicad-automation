"""Renames the first Camera Set GetCameras lists to "Renderer views" and makes it the active Camera Set with ModifyCameraSets."""
import aclib

cameraSets = aclib.RunTapirCommand ('GetCameras')['cameraSets']
if cameraSets:
    response = aclib.RunTapirCommand ('ModifyCameraSets', {
        'cameraSets': [
            {
                'cameraSetElementId': cameraSets[0]['cameraSetElementId'],
                'name': 'Renderer views',
                'isActive': True
            }
        ]
    })
    aclib.PrintJson (response)
