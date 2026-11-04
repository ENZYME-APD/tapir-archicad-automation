"""Moves a Favorite into a folder of the Favorites palette with MoveFavorites.

Needs a Favorite. The moved Favorite is a copy of the first Favorite and the folder is new; both are created by the
script and deleted at the end. Requires Archicad 29 or later.
"""
import aclib

favorites = aclib.RunTapirCommand ('GetAllFavorites')['favorites']
if not favorites:
    raise SystemExit ('The project has no Favorite.')

created = aclib.RunTapirCommand ('CreateFavorite', {
    'name': 'Tapir Example Favorite',
    'source': {'favoriteName': favorites[0]['name']},
    'conflictPolicy': 'RenameNew'
})
aclib.RunTapirCommand ('CreateFavoriteFolder', {'parentPath': [], 'name': 'Tapir Example Folder'})

aclib.PrintJson (aclib.RunTapirCommand ('MoveFavorites', {
    'favorites': [created['name']],
    'targetPath': ['Tapir Example Folder']
}))

# Deleting the folder deletes the moved Favorite with it.
aclib.RunTapirCommand ('DeleteFavoriteFolder', {'path': ['Tapir Example Folder'], 'recursive': True})
