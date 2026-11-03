import aclib

# Reads the stored settings of the Window Favorites without applying them or placing an
# element: element type, folder, library part and GDL parameters, the marker (library part
# and parameters) and the sub-elements. A Favorite without a marker has no 'marker' field.

windowFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {
    'elementType': 'Window'
}, debug=False)

aclib.RunTapirCommand ('GetFavoriteSettings', {
    'favorites': windowFavorites.get ('favorites', [])[:1]
})
