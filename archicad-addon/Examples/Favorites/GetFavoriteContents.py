"""Lists every Favorite with its folder path, classifications, element category and property values with GetFavoriteContents.

Give 'folderPath', the folder names from the root down, to read only one folder and its subfolders.
Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetFavoriteContents', {}))
