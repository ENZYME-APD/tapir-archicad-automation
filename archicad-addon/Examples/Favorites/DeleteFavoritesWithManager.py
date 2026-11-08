"""Deletes a Favorite through the Favorite manager with DeleteFavoritesWithManager.

Needs at least one Wall. The deleted Favorite is created from the first wall by the script first;
the second, missing Favorite shows the error item. Requires Archicad 29 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': walls[0]['elementId'], 'favorite': 'Tapir Example Wall'}]
})

# 'folderPath' is optional: without it the Favorite is found by name anywhere,
# with it only directly in that folder (here the root folder).
aclib.PrintJson (aclib.RunTapirCommand ('DeleteFavoritesWithManager', {
    'favorites': [
        {'name': 'Tapir Example Wall', 'folderPath': []},
        {'name': 'No Such Favorite'}
    ]
}))
