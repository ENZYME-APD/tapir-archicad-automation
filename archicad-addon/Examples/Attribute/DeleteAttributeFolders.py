"""Creates a temporary Layer folder and deletes it again with DeleteAttributeFolders.

Requires Archicad 26 or later.
"""
import aclib

folder = {'attributeType': 'Layer', 'path': ['Tapir Example Folder To Delete']}

aclib.RunTapirCommand ('CreateAttributeFolders', {'attributeFolders': [folder]})
aclib.PrintJson (aclib.RunTapirCommand ('DeleteAttributeFolders', {'attributeFolders': [folder]}))
