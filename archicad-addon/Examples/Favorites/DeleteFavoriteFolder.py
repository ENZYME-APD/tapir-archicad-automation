"""Deletes a folder of the Favorites palette with DeleteFavoriteFolder.

The folder is created by the script first. A folder that is not empty is only deleted with recursive set to true,
and then its Favorites are deleted with it. Requires Archicad 29 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Folder'})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteFavoriteFolder', {
    'path': ['Tapir Example Folder'],
    'recursive': False
}))
