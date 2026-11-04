"""Lists the Favorites stored in a .prefs file with ReadFavoritesFile, without importing them.

Needs a Wall Favorite: the script first exports the Wall Favorites to a temporary file to have a file to read.
"""
import os
import tempfile

import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

with tempfile.TemporaryDirectory () as tempDir:
    favoritesFile = os.path.join (tempDir, 'WallFavorites.prefs')
    aclib.RunTapirCommand ('ExportFavorites', {'path': favoritesFile, 'names': wallFavorites})

    aclib.PrintJson (aclib.RunTapirCommand ('ReadFavoritesFile', {'path': favoritesFile}))
