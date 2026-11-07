"""Renames a Favorite with RenameFavorites.

Needs at least one Wall. The Favorite is created from the first wall by the script and deleted at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': 'Tapir Example Wall'}]
})

aclib.PrintJson (aclib.RunTapirCommand ('RenameFavorites', {
    'renames': [{'oldName': 'Tapir Example Wall', 'newName': 'Tapir Example Wall (renamed)'}]
}))

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': ['Tapir Example Wall (renamed)']})
