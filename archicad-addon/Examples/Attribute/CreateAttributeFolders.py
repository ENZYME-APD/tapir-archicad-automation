"""Creates the Layer folder 'Tapir Example/Subfolder' with CreateAttributeFolders.

The missing parent folder is created too. Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateAttributeFolders', {
    'attributeFolders': [{'attributeType': 'Layer', 'path': ['Tapir Example', 'Subfolder']}]
}))
