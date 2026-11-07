"""Copies the 2D symbol of a door into a library part and deletes its preview picture with SetLibraryPartPictures.

The library part is created in the embedded library by the script and deleted at the end. Requires Archicad 28 or later.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

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

aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartPictures', {
    'name': libraryPartName,
    'copy2DSymbolFrom': doors[0]['fileName'],
    'deletePreviewPicture': True
}))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
