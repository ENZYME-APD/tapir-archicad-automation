"""Asks the user where to save a door library part with PickLibraryPartSaveLocation.

The dialog starts from Door.gsm in the project's embedded library (give defaultPath to start elsewhere).
The response tells whether the user cancelled; if not, it has the chosen location. Nothing is saved.
"""
import aclib

result = aclib.RunTapirCommand ('PickLibraryPartSaveLocation', {
    'libraryPartType': 'Door',
    'browseForFolder': True,
    'title': 'Save the generated door'
})
if result is not None:
    print ('Cancelled.' if result['cancelled'] else 'Save the door to: ' + result['path'])
