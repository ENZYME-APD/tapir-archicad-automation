import aclib

# Lists every Favorite of the project with its element type (Archicad 29 or newer),
# then only the Wall Favorites.

aclib.RunTapirCommand ('GetAllFavorites', {})

aclib.RunTapirCommand ('GetAllFavorites', {
    'elementType': 'Wall'
})
