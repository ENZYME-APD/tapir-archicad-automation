"""Renames a folder of the Favorites palette with RenameFavoriteFolder.

The folder is created by the script and deleted at the end. Requires Archicad 29 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Folder'}, debug = False)

aclib.RunTapirCommand ('RenameFavoriteFolder', {
    'path': ['Tapir Example Folder'],
    'newName': 'Tapir Example Folder (renamed)'
})

aclib.RunTapirCommand ('DeleteFavoriteFolder', {'path': ['Tapir Example Folder (renamed)']}, debug = False)
