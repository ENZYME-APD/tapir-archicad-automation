import os
import tempfile
import aclib

# Saves what the current window shows as an image file: the pens, layers, scale and
# graphic overrides of the window as it is on screen. The format comes from the
# extension when it is not given. For a 2D window the response also carries the zoom
# box and rotation of the window, the data GetView2DTransformations reports, so the
# pixels of the image can be mapped back to model coordinates.

aclib.RunTapirCommand ('SaveCurrentWindowAsImage', {
    'imageFilePath': os.path.join (tempfile.gettempdir (), 'tapir_example.png')
})

aclib.RunTapirCommand ('SaveCurrentWindowAsImage', {
    'imageFilePath': os.path.join (tempfile.gettempdir (), 'tapir_example.jpg'),
    'format': 'jpg',
    'colorDepth': '256Grays',
    'entireDrawing': True
})
