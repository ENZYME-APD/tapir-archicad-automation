"""Prints the folder tree of the Favorites palette with GetFavoriteFolders.

Requires Archicad 29 or later.
"""
import aclib

rootFolder = aclib.RunTapirCommand ('GetFavoriteFolders')['rootFolder']

folders = [rootFolder]
while folders:
    folder = folders.pop (0)
    print ('/'.join (folder['path']) or '(root)', '-', len (folder['favorites']), 'Favorites')
    folders += folder['subFolders']
