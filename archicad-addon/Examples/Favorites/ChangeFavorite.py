"""Gives a Favorite the settings of another Favorite with ChangeFavorite.

Needs a Wall Favorite. The changed Favorite is a copy of the first Wall Favorite, created by the script and deleted at
the end; it gets the settings of the last Wall Favorite. Requires Archicad 29 or later.
"""
import aclib

wallFavorites = aclib.RunTapirCommand ('GetAllFavorites', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

created = aclib.RunTapirCommand ('CreateFavorite', {
    'name': 'Tapir Example Favorite',
    'source': {'favoriteName': wallFavorites[0]['name']},
    'conflictPolicy': 'RenameNew'
})

aclib.PrintJson (aclib.RunTapirCommand ('ChangeFavorite', {
    'favoriteName': created['name'],
    'source': {'favoriteName': wallFavorites[-1]['name']}
}))

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [created['name']]})
