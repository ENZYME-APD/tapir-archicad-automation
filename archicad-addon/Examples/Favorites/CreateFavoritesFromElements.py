"""Creates a Favorite from the first wall of the project with CreateFavoritesFromElements, then deletes it.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{
        'elementId': walls[0]['elementId'],
        'favorite': 'Tapir Example Wall'
    }]
}))

aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': ['Tapir Example Wall']})
