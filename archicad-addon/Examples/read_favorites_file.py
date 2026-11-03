import os
import tempfile
import aclib

# Lists the Favorites of a .prefs file or folder without importing them, so they can be
# compared with the project's Favorites before ImportFavorites overwrites any of them.
# Here the project's own Favorites are exported first, then read back from the file.

path = os.path.join (tempfile.gettempdir (), 'tapir_example_read_favorites.prefs')

aclib.RunTapirCommand ('ExportFavorites', {
    'path': path
})

aclib.RunTapirCommand ('ReadFavoritesFile', {
    'path': path
})
