"""Saves the 3D preview image of the first Wall Favorite as a PNG file with GetFavoritePreviewImage.

Needs a Wall Favorite. The image is written into a new temporary folder.
"""
import base64
import os
import tempfile

import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

response = aclib.RunTapirCommand ('GetFavoritePreviewImage', {
    'favorite': wallFavorites[0],
    'imageType': '3D',
    'format': 'png',
    'width': 256,
    'height': 256
})

imagePath = os.path.join (tempfile.mkdtemp (), 'favorite_preview.png')
with open (imagePath, 'wb') as imageFile:
    imageFile.write (base64.b64decode (response['previewImage']))
print ('Preview image saved to', imagePath)
