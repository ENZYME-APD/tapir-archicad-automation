"""Lets the user choose or create a library folder with PickLibraryFolder.

The response tells whether the user cancelled; if not, it has the location of the folder.
The folder is not added to the project's libraries; call AddLibraries with the path for that.
"""
import aclib

result = aclib.RunTapirCommand ('PickLibraryFolder', {'defaultName': 'Script Library'})
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'Chosen library folder: ' + result['path'])
