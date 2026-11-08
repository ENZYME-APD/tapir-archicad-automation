"""Sets the author and keywords of a library part with SetLibraryPartMetadata and reads them back.

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

aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartMetadata', {
    'libraryParts': [{
        'name': libraryPartName,
        'metadata': {
            'author': 'Tapir',
            'keywords': 'tapir example box'
        }
    }]
}))

aclib.RunTapirCommand ('GetLibraryPartMetadata', {'libraryParts': [{'name': libraryPartName}]})

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
