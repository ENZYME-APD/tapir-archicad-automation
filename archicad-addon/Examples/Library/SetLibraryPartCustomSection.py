"""Writes a custom section into a library part with SetLibraryPartCustomSection, reads it back and deletes it.

The library part is created in the embedded library by the script and deleted at the end. Requires Archicad 28 or later.
"""
import base64

import aclib

# A library part of the embedded library, created by this script and deleted at the end.
createdPart = aclib.RunTapirCommand ('CreateLibraryParts', {
    'libraryParts': [{
        'name': 'Tapir Example Box',
        'parentName': 'General GDL Object',
        'path': 'TapirExamples/Tapir Example Box.gsm',
        'script3D': 'BLOCK A, B, 1'
    }],
    'overwriteExisting': True
})['libraryParts'][0]
if 'error' in createdPart:
    raise SystemExit (createdPart['error']['message'])
libraryPartName = 'Tapir Example Box.gsm'

# A four-character section code is given as its integer value, the first character in the highest byte.
sectionId = int.from_bytes (b'TAPR', 'big')

aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartCustomSection', {
    'name': libraryPartName,
    'sectionId': sectionId,
    'dataVersion': 1,
    'data': base64.b64encode (b'Data stored by Tapir').decode ('ascii')
}))

aclib.RunTapirCommand ('GetLibraryPartCustomSection', {'name': libraryPartName, 'sectionId': sectionId})

aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartCustomSection', {'name': libraryPartName, 'sectionId': sectionId, 'delete': True}))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
