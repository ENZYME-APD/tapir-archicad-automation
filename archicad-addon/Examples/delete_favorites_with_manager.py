import aclib

# Creates a Favorite from the first wall and deletes it again through the Favorite manager
# (Archicad 29 or newer). 'folderPath' is optional: without it the Favorite is found by name
# anywhere, with it only directly in that folder (here the root folder).

walls = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Wall'
}, debug=False)['elements']

if len (walls) == 0:
    print ('No walls in the project; nothing to create a Favorite from.')
else:
    aclib.RunTapirCommand ('CreateFavoritesFromElements', {
        'favoritesFromElements': [{
            'elementId': walls[0]['elementId'],
            'favorite': 'WallToDeleteFromPython'
        }]
    }, debug=False)

    aclib.RunTapirCommand ('DeleteFavoritesWithManager', {
        'favorites': [
            {'name': 'WallToDeleteFromPython', 'folderPath': []},
            {'name': 'NoSuchFavoriteFromPython'}
        ]
    })
