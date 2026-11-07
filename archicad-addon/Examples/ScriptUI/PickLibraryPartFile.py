"""Lets the user pick a library part in Archicad's Open Library Part dialog with PickLibraryPartFile.

The response tells whether the user cancelled; if not, it has the location of the chosen library part.
Set generalGDLObjectsOnly to filter the dialog to General GDL Objects.
"""
import aclib

result = aclib.RunTapirCommand ('PickLibraryPartFile', {'generalGDLObjectsOnly': True})
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'Chosen library part: ' + result['path'])
