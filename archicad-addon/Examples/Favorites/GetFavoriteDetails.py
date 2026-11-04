"""Reads the classifications, property values and element categories stored in the Wall Favorites with GetFavoriteDetails.

Needs a Wall Favorite.
"""
import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

aclib.PrintJson (aclib.RunTapirCommand ('GetFavoriteDetails', {'favorites': wallFavorites[:3]}))
