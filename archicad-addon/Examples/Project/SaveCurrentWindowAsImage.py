"""Saves what the current window shows as a png in the temp folder with SaveCurrentWindowAsImage.

Master layout windows are not supported.
"""
import os
import tempfile
import aclib

aclib.RunTapirCommand ('SaveCurrentWindowAsImage', {
    'imageFilePath': os.path.join (tempfile.mkdtemp (), 'TapirWindow.png'),
    'format': 'png',
    'colorDepth': 'trueColor',
    'entireDrawing': True
})
