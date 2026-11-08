"""Deletes a Favorite with DeleteFavorites.

Needs at least one Wall. The deleted Favorite is created from the first wall by the script first.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': 'Tapir Example Wall'}]
})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteFavorites', {'favorites': ['Tapir Example Wall']}))
