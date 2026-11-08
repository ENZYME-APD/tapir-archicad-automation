"""Copies the library part of the first object in the project into a temporary folder with CopyLibraryParts.

Needs at least one Object. The copy gets a new unique ID; the temporary folder is deleted at the
end. Requires Archicad 28 or later.
"""
import shutil
import tempfile

import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements'][:1]
if not objects:
    raise SystemExit ('No object in the project.')

targetFolder = tempfile.mkdtemp ()
try:
    aclib.PrintJson (aclib.RunTapirCommand ('CopyLibraryParts', {
        'libraryParts': [{
            'elementId': objects[0]['elementId'],
            'newName': 'Tapir example copy.gsm',
            'targetFolder': targetFolder
        }]
    }))
finally:
    shutil.rmtree (targetFolder, ignore_errors = True)
