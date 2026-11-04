"""Creates a temporary Layer folder, renames it with RenameAttributeFolders, then deletes it.

Requires Archicad 26 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateAttributeFolders', {
    'attributeFolders': [{'attributeType': 'Layer', 'path': ['Tapir Example Folder']}]
})

aclib.PrintJson (aclib.RunTapirCommand ('RenameAttributeFolders', {
    'attributeFolders': [{'attributeType': 'Layer', 'path': ['Tapir Example Folder'], 'newName': 'Tapir Renamed Folder'}]
}))

aclib.RunTapirCommand ('DeleteAttributeFolders', {
    'attributeFolders': [{'attributeType': 'Layer', 'path': ['Tapir Renamed Folder']}]
})
