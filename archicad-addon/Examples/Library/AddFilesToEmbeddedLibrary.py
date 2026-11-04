"""Adds a small PNG file to the embedded library with AddFilesToEmbeddedLibrary.

The image is written to a temporary folder first and deleted from the embedded library at the end
(the deletion requires Archicad 27 or later).
"""
import base64
import os
import tempfile

import aclib

PNG_1X1 = base64.b64decode ('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==')
outputPath = 'TapirExamples/tapir_example.png'

with tempfile.TemporaryDirectory () as tempDir:
    inputPath = os.path.join (tempDir, 'tapir_example.png')
    with open (inputPath, 'wb') as pngFile:
        pngFile.write (PNG_1X1)

    aclib.PrintJson (aclib.RunTapirCommand ('AddFilesToEmbeddedLibrary', {
        'files': [{'inputPath': inputPath, 'outputPath': outputPath, 'type': 'Pict'}],
        'overwriteExisting': True
    }))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': outputPath}]})
