"""Lets the user pick a layer folder with PickAttributeFolder and waits for the user (Archicad 26 and later).

A small dialog shows Archicad's attribute folder picker; the response is the chosen folder as its path from the
root folder (an empty path is the root folder), or cancelled. The folder can be the targetFolder of
MoveAttributesAndFolders.
"""
import aclib

result = aclib.RunTapirCommand ('PickAttributeFolder', {'attributeType': 'Layer'})
if result is not None:
    if result['cancelled']:
        print ('Cancelled.')
    else:
        print ('Chosen folder: /' + '/'.join (result['attributeFolder']['path']))
