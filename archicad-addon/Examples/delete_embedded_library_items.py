import base64
import os
import tempfile

import aclib

# A 1x1 transparent png; AddFilesToEmbeddedLibrary embeds files as 'Pict' type by default.
PNG_1X1 = base64.b64decode ('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==')

embeddedPath = 'TapirExamples/tapir_example.png'

with tempfile.TemporaryDirectory () as tempDir:
    inputPath = os.path.join (tempDir, 'tapir_example.png')
    with open (inputPath, 'wb') as pngFile:
        pngFile.write (PNG_1X1)

    response = aclib.RunTapirCommand ('AddFilesToEmbeddedLibrary', {
        'files': [{
            'inputPath': inputPath,
            'outputPath': embeddedPath
        }]
    }, debug = False)
    print (response)

    # Re-adding under the same outputPath with overwriteExisting replaces the loaded part
    # instead of leaving the previously loaded version in use (Archicad 27 or newer).
    response = aclib.RunTapirCommand ('AddFilesToEmbeddedLibrary', {
        'files': [{
            'inputPath': inputPath,
            'outputPath': embeddedPath
        }],
        'overwriteExisting': True
    }, debug = False)
    print (response)

# The path is the same relative path that AddFilesToEmbeddedLibrary takes as outputPath.
response = aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {
    'embeddedLibraryItems': [{
        'path': embeddedPath
    }]
}, debug = False)
print (response)
