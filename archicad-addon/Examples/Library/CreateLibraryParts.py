"""Creates a GDL box object in the embedded library with CreateLibraryParts, then deletes it again.

The parent subtype is the 'General GDL Object' of the loaded Archicad Library. Deleting the part at the
end requires Archicad 27 or later.
"""
import aclib

response = aclib.RunTapirCommand ('CreateLibraryParts', {
    'libraryParts': [{
        'name': 'Tapir Example Box',
        'parentName': 'General GDL Object',
        'path': 'TapirExamples/Tapir Example Box.gsm',
        'a': 0.6,
        'b': 0.4,
        'parameters': [
            {'name': 'boxHeight', 'type': 'Length', 'value': 0.5, 'description': 'Box Height'},
            {'name': 'boxLabel', 'type': 'String', 'value': 'Tapir', 'description': 'Label'}
        ],
        'script2D': 'RECT2 0, 0, A, B\nTEXT2 A / 2, B / 2, boxLabel',
        'script3D': 'BLOCK A, B, boxHeight',
        'parameterScript': 'VALUES "boxHeight" RANGE [0.1, 3.0]'
    }],
    'overwriteExisting': True
})
aclib.PrintJson (response)
createdPart = response['libraryParts'][0]
if 'error' in createdPart:
    raise SystemExit (createdPart['error']['message'])

aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {'embeddedLibraryItems': [{'path': createdPart['path']}]})
