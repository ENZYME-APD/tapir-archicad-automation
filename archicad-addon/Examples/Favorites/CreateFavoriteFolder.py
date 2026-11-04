"""Creates a folder and a subfolder in the Favorites palette with CreateFavoriteFolder, then deletes them.

Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Folder'}))
aclib.PrintJson (aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': ['Tapir Example Folder'], 'name': 'Walls'}))

aclib.RunTapirCommand ('DeleteFavoriteFolder', {'path': ['Tapir Example Folder'], 'recursive': True})
