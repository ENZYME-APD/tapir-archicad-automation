"""Moves a folder of the Favorites palette into another folder with MoveFavoriteFolder.

Both folders are created by the script and deleted at the end. Requires Archicad 29 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Target'})
aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Moved'})

aclib.PrintJson (aclib.RunTapirCommand ('MoveFavoriteFolder', {
    'folder': ['Tapir Example Moved'],
    'targetFolder': ['Tapir Example Target']
}))

aclib.RunTapirCommand ('DeleteFavoriteFolder', {'path': ['Tapir Example Target'], 'recursive': True})
