"""Imports a Favorite from a .prefs file with ImportFavorites.

Needs at least one Wall. The script creates the Favorite 'Tapir Example Wall' from the first wall, exports it to a
temporary file, deletes it, imports it again from the file and finally deletes it.
"""
import os
import tempfile

import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

favoriteName = 'Tapir Example Wall'
aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': favoriteName}]
})

with tempfile.TemporaryDirectory () as tempDir:
    favoritesFile = os.path.join (tempDir, 'TapirExample.prefs')
    aclib.RunTapirCommand ('ExportFavorites', {'path': favoritesFile, 'names': [favoriteName]})
    aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [favoriteName]})

    aclib.PrintJson (aclib.RunTapirCommand ('ImportFavorites', {
        'path': favoritesFile,
        'format': 'prefs',
        'conflictPolicy': 'Skip'
    }))

aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [favoriteName]})
