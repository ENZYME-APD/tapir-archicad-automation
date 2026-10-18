import os
import tempfile
import aclib

# Writes the project's Favorites to a .prefs file. Without a 'names' list the command
# exports every Favorite, as Export Favorites in the Favorites palette does.
# The command answers with an ExecutionResult: {"success": true} or
# {"success": false, "error": {...}}.

aclib.RunTapirCommand ('ExportFavorites', {
    'path': os.path.join (tempfile.gettempdir (), 'tapir_example_favorites.prefs')
})
