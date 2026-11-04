"""Applies the first Wall Favorite to a new wall with ApplyFavoritesToElements, keeping the wall's height.

Needs a Wall Favorite. The wall is created by the script and deleted at the end.
"""
import aclib

wallFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'})['favorites']
if not wallFavorites:
    raise SystemExit ('The project has no Wall Favorite.')

wall = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 5.0, 'y': 0.0}, 'height': 3.0}]
})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ApplyFavoritesToElements', {
    'favoritesToApply': [{'elementId': wall['elementId'], 'favorite': wallFavorites[0]}],
    'applySettings': True,
    'applyClassifications': True,
    'applyCategories': True,
    'applyProperties': False,
    'keepFields': ['height', 'topOffset', 'relativeTopStory']
}))

aclib.RunTapirCommand ('DeleteElements', {'elements': [wall]})
