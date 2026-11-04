"""Adds a small PNG file to the embedded library, then deletes it again with DeleteEmbeddedLibraryItems.

Requires Archicad 27 or later.
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
    aclib.RunTapirCommand ('AddFilesToEmbeddedLibrary', {
        'files': [{'inputPath': inputPath, 'outputPath': outputPath}],
        'overwriteExisting': True
    })

# The path is the same relative path AddFilesToEmbeddedLibrary took as outputPath.
aclib.PrintJson (aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {
    'embeddedLibraryItems': [{'path': outputPath}]
}))
