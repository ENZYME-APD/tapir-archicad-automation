import aclib
import base64
import os
import tempfile

# Returns what the 3D window shows as a base64 encoded image, with its size in pixels,
# without writing a file as SaveCurrentWindowAsImage does. With 'source': 'RenderingWindow'
# it returns the image of the PhotoRendering window instead; 'format': 'jpg' gives a jpg.
# Available from Archicad 30. The 3D window must be open, otherwise the command fails.

response = aclib.RunTapirCommand ('GetCurrentViewImage', {
    'source': '3DWindow',
    'format': 'png'
}, debug = False)

if response is not None and 'image' in response:
    filePath = os.path.join (tempfile.gettempdir (), 'tapir_example_3d_window.png')
    with open (filePath, 'wb') as imageFile:
        imageFile.write (base64.b64decode (response['image']))
    print ('Saved the {}x{} image of the 3D window to {}'.format (response['width'], response['height'], filePath))
