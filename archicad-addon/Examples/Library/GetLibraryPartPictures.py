"""Saves the preview picture of the first loaded door library part with GetLibraryPartPictures.

The picture is written as a PNG file into a new temporary folder. Requires Archicad 28 or later.
"""
import base64
import os
import tempfile

import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

response = aclib.RunTapirCommand ('GetLibraryPartPictures', {'name': doors[0]['fileName']})
print ('Number of GDL pictures:', len (response['gdlPictures']))
if 'previewPicture' in response:
    previewPath = os.path.join (tempfile.mkdtemp (), 'preview.png')
    with open (previewPath, 'wb') as previewFile:
        previewFile.write (base64.b64decode (response['previewPicture']))
    print ('Preview picture saved to', previewPath)
