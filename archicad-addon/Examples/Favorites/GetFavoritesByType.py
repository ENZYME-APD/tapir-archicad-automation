"""Lists the names of the Wall Favorites of the project with GetFavoritesByType."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'}))
