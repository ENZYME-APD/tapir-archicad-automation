"""Sets the A and B parameters of a library part with SetLibraryPartParameters.

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

# The parameters in the format of SetGDLParametersOfElements: only the given ones are changed.
aclib.PrintJson (aclib.RunTapirCommand ('SetLibraryPartParameters', {
    'libraryParts': [{
        'name': 'Tapir Example Box.gsm',
        'parameters': [
            {'name': 'A', 'value': 1.2},
            {'name': 'B', 'value': 0.6}
        ]
    }]
}))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
