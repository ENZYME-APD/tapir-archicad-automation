"""Renders the 3D model with the current rendering scene into a 640x480 png in the temp folder with PhotoRender.

The model is rendered as the 3D window shows it. The scene's own size and resolution are not changed.
"""
import os
import tempfile
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('PhotoRender', {
    'imageFilePath': os.path.join (tempfile.mkdtemp (), 'TapirRender.png'),
    'format': 'png',
    'width': 640,
    'height': 480,
    'resolution': 72
}))
