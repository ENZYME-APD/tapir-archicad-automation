import os
import tempfile
import aclib

# Exports the project's Favorites in the two formats Archicad 29 added: one file per
# Favorite into a folder (xml), and a Favorites file of earlier Archicad versions
# (legacyPRF). Each answers with an ExecutionResult.

xmlFolder = os.path.join (tempfile.gettempdir (), 'tapir_example_favorites')
os.makedirs (xmlFolder, exist_ok=True)

aclib.RunTapirCommand ('ExportFavorites', {
    'path': xmlFolder,
    'format': 'xml'
})

aclib.RunTapirCommand ('ExportFavorites', {
    'path': os.path.join (tempfile.gettempdir (), 'tapir_example_favorites.prf'),
    'format': 'legacyPRF'
})
