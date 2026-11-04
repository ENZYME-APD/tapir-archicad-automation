"""Lists every Favorite of the project with its element type with GetAllFavorites.

Requires Archicad 29 or later.
"""
import aclib

favorites = aclib.RunTapirCommand ('GetAllFavorites')['favorites']
for favorite in favorites:
    print (favorite.get ('elementType'), favorite['name'])

# Only the Favorites of one element type.
aclib.PrintJson (aclib.RunTapirCommand ('GetAllFavorites', {'elementType': 'Door'}))
