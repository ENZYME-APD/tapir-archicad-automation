"""Makes the settings of the first Favorite of the project the settings of its tool with ApplyFavoriteToToolDefault.

Needs a Favorite. Changes the default settings of that tool. Requires Archicad 29 or later.
"""
import aclib

favorites = aclib.RunTapirCommand ('GetAllFavorites')['favorites']
if not favorites:
    raise SystemExit ('The project has no Favorite.')

aclib.PrintJson (aclib.RunTapirCommand ('ApplyFavoriteToToolDefault', {'favoriteName': favorites[0]['name']}))
