"""Copies two PNG files into a folder of the embedded library with CopyFilesIntoEmbeddedLibrary.

The files are written to a temporary folder first; the copies are deleted from the embedded library at the end
(the deletion requires Archicad 27 or later).
"""
import base64
import os
import tempfile

import aclib

PNG_1X1 = base64.b64decode ('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==')
targetFolder = 'TapirExamples/Copied'
fileNames = ['tapir_example_a.png', 'tapir_example_b.png']

with tempfile.TemporaryDirectory () as tempDir:
    files = []
    for fileName in fileNames:
        path = os.path.join (tempDir, fileName)
        with open (path, 'wb') as pngFile:
            pngFile.write (PNG_1X1)
        files.append ({'path': path})

    aclib.PrintJson (aclib.RunTapirCommand ('CopyFilesIntoEmbeddedLibrary', {
        'files': files,
        'targetFolder': targetFolder,
        'overwriteIfExists': True
    }))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {
    'embeddedLibraryItems': [{'path': targetFolder + '/' + fileName} for fileName in fileNames]
})
