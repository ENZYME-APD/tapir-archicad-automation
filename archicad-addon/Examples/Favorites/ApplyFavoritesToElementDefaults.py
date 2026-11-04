"""Makes the first Wall Favorite the default settings of the Wall tool with ApplyFavoritesToElementDefaults.

Needs a Wall Favorite. Changes the Wall tool's default settings.
"""
import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

aclib.PrintJson (aclib.RunTapirCommand ('ApplyFavoritesToElementDefaults', {'favorites': [wallFavorites[0]]}))
