"""Creates a copy of the first Favorite of the project with CreateFavorite, then deletes it.

Needs a Favorite. RenameNew picks a free name when the name is taken. Requires Archicad 29 or later.
"""
import aclib

favorites = aclib.RunTapirCommand ('GetAllFavorites')['favorites']
if not favorites:
    raise SystemExit ('The project has no Favorite.')

created = aclib.RunTapirCommand ('CreateFavorite', {
    'name': 'Tapir Example Favorite',
    'folder': [],
    'source': {'favoriteName': favorites[0]['name']},
    'conflictPolicy': 'RenameNew'
})
aclib.PrintJson (created)

aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [created['name']]})
