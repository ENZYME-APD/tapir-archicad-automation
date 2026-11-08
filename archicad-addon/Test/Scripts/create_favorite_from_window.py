import aclib

# Example for issue #397: a Favorite created from a window that was placed with the
# Window tool must show up under the Window section of the Favorites palette, not under
# Corner Window. Some standard library window parts (the Hungarian "ablak" family among
# them) are dual-use GDL parts that the Corner Window tool can place too, and for those
# the element header reports the corner window variation regardless of how the element
# was actually placed - CreateFavoritesFromElements now corrects that using the part's
# AC_CW_Function GDL parameter.
#
# The Favorites palette section a Favorite lands in is not readable through the JSON
# interface (GetFavoritesByType filters by element type only, not by variation), so the
# section itself has to be checked by eye in Archicad: run this against a project with a
# dual-use window part placed as an ordinary window and confirm that 'WindowFromPython'
# appears under Window / Ablak.

allWindows = aclib.RunTapirCommand(
    'GetElementsByType', {
        'elementType': 'Window'
    })['elements']

if len(allWindows) == 0:
    print('No windows in the project; nothing to create a Favorite from.')
else:
    firstWindow = allWindows[0]

    aclib.RunTapirCommand(
        'CreateFavoritesFromElements', {
            'favoritesFromElements': [{
                'elementId': firstWindow['elementId'],
                'favorite': 'WindowFromPython'
            }]
        })

    windowFavorites = aclib.RunTapirCommand(
        'GetFavoritesByType', {
            'elementType': 'Window'
        })['favorites']
    print('Listed among Window favorites: {}'.format('WindowFromPython' in windowFavorites))

    aclib.RunTapirCommand(
        'DeleteFavorites', {
            'favorites': ['WindowFromPython']
        })
