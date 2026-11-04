"""Updates a Favorite from the settings of a wall with UpdateFavoritesFromElements.

Needs at least one Wall. The Favorite 'Tapir Example Wall' is created by the script and deleted at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

favoriteName = 'Tapir Example Wall'
aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': favoriteName}]
})

aclib.PrintJson (aclib.RunTapirCommand ('UpdateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': favoriteName}]
}))

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [favoriteName]})
