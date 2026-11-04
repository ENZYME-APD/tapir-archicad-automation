"""Sets the 2D and 3D scripts of a library part with SetLibraryPartScripts and reads them back.

The library part is created in the embedded library by the script and deleted at the end. Requires Archicad 28 or later.
"""
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

aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartScripts', {
    'libraryParts': [{
        'name': libraryPartName,
        'scripts': {
            '2d': 'RECT2 0, 0, A, B',
            '3d': 'BLOCK A, B, 2'
        },
        'deleteScripts': ['properties']
    }]
}))

aclib.RunTapirCommand ('GetLibraryPartScripts', {
    'libraryParts': [{'libraryPartName': libraryPartName}],
    'scripts': ['2d', '3d', 'properties']
})

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
