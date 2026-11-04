"""Exports the Wall Favorites of the project to a .prefs file with ExportFavorites.

Needs a Wall Favorite. The file is written into a new temporary folder.
"""
import os
import tempfile

import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

favoritesFile = os.path.join (tempfile.mkdtemp (), 'WallFavorites.prefs')

aclib.PrintJson (aclib.RunTapirCommand ('ExportFavorites', {
    'path': favoritesFile,
    'names': wallFavorites,
    'format': 'prefs'
}))
print ('Favorites exported to', favoritesFile)
