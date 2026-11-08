"""Searches the Favorites whose name or folder contains "Wall" with SearchFavorites."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SearchFavorites', {'query': 'Wall'}))
