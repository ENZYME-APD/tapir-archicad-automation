"""Gets what the 3D window shows as a png with GetCurrentViewImage and writes it to the temp folder.

Needs the 3D window to be open. Requires Archicad 30 or later.
"""
import base64
import os
import tempfile
import aclib

response = aclib.RunTapirCommand ('GetCurrentViewImage', {'source': '3DWindow', 'format': 'png'})
if not response or 'image' not in response:
    raise SystemExit ('No image: {}'.format (response))

filePath = os.path.join (tempfile.mkdtemp (), 'Tapir3DWindow.png')
with open (filePath, 'wb') as imageFile:
    imageFile.write (base64.b64decode (response['image']))
print ('Saved the {}x{} image to {}'.format (response['width'], response['height'], filePath))
