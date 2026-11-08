"""Saves the 3D preview image of the first wall as a PNG file in the temp folder with GetElementPreviewImage.

Needs at least one Wall. The image comes base64 encoded.
"""
import aclib
import base64
import os
import tempfile

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

response = aclib.RunTapirCommand ('GetElementPreviewImage', {
    'elementId': walls[0]['elementId'],
    'imageType': '3D',
    'format': 'png',
    'width': 256,
    'height': 256
})

path = os.path.join (tempfile.mkdtemp (), 'wall_preview.png')
with open (path, 'wb') as file:
    file.write (base64.b64decode (response['previewImage']))
print ('Saved ' + path)
