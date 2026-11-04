"""Replaces the 3D script of a library part with UpdateLibraryPartSections.

The library part is created in the embedded library by the script and deleted at the end.
Only sections the library part already has are replaced.
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

aclib.PrintJson (aclib.RunTapirCommand ('UpdateLibraryPartSections', {
    'name': 'Tapir Example Box',
    'sections': [
        {'type': '3DScript', 'text': 'BLOCK A, B, ZZYZX\n'}
    ]
}))

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
