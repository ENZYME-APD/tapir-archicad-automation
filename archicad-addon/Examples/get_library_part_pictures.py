import aclib
import base64
import os
import tempfile

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
libraryPartName = 'My Object.gsm'

response = aclib.RunTapirCommand ('GetLibraryPartPictures', {
    'name': libraryPartName
}, debug = False)

if response and 'previewPicture' in response:
    # The pictures are base64 encoded PNGs; save the preview picture next to the temporary files.
    previewPath = os.path.join (tempfile.gettempdir (), 'library_part_preview.png')
    with open (previewPath, 'wb') as previewFile:
        previewFile.write (base64.b64decode (response['previewPicture']))
    print ('Preview picture saved to ' + previewPath)

if response and 'gdlPictures' in response:
    print ('Number of GDL pictures: {}'.format (len (response['gdlPictures'])))
else:
    print (response)
