import base64
import os
import tempfile

import aclib

# A 1x1 transparent png, copied into the embedded library under two names.
PNG_1X1 = base64.b64decode ('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==')

targetFolder = 'TapirExamples/Organized'
fileNames = ['tapir_example_a.png', 'tapir_example_b.png']

# Creates the folder and its missing parent folders inside the embedded library.
response = aclib.RunTapirCommand ('CreateFolderInEmbeddedLibrary', {
    'folderPath': targetFolder
}, debug = False)
print (response)

with tempfile.TemporaryDirectory () as tempDir:
    files = []
    for fileName in fileNames:
        inputPath = os.path.join (tempDir, fileName)
        with open (inputPath, 'wb') as pngFile:
            pngFile.write (PNG_1X1)
        files.append ({'path': inputPath})

    # Copies all files into the folder with one library operation; they keep their names.
    response = aclib.RunTapirCommand ('CopyFilesIntoEmbeddedLibrary', {
        'files': files,
        'targetFolder': targetFolder
    }, debug = False)
    print (response)

    # Copying again replaces the items of the same name.
    response = aclib.RunTapirCommand ('CopyFilesIntoEmbeddedLibrary', {
        'files': files,
        'targetFolder': targetFolder,
        'overwriteIfExists': True
    }, debug = False)
    print (response)

# Clean up the copied items (Archicad 27 or newer).
response = aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {
    'embeddedLibraryItems': [{'path': targetFolder + '/' + fileName} for fileName in fileNames]
}, debug = False)
print (response)
