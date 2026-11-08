"""Lets the user pick a picture in Archicad's Open Picture dialog with PickPictureFile.

The response tells whether the user cancelled; if not, it has the location of the chosen picture.
Archicad adds the picture to the embedded library when it is not in it yet.
"""
import aclib

result = aclib.RunTapirCommand ('PickPictureFile')
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'Chosen picture: ' + result['path'])
