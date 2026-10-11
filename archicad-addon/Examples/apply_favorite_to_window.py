import aclib

# A Favorite made from a window keeps the window's marker (#812), and applying a Favorite to a
# placed window changes its library part, parameters and size while the window stays in its wall
# at its position. The first window of the project is captured and applied to the last one.

windows = aclib.RunTapirCommand ('GetElementsByType', { 'elementType': 'Window' })['elements']

if len (windows) == 0:
    print ('No windows in the project.')
else:
    sourceWindow = windows[0]
    targetWindow = windows[-1]

    def GetDetails (window):
        return aclib.RunTapirCommand ('GetDetailsOfElements', { 'elements': [window] }, debug = False)['detailsOfElements'][0]

    detailsBefore = GetDetails (targetWindow)

    aclib.RunTapirCommand ('CreateFavoritesFromElements', { 'favoritesFromElements': [{
        'elementId': sourceWindow['elementId'],
        'favorite': 'WindowFavoriteFromPython'
    }] })

    aclib.RunTapirCommand ('ApplyFavoritesToElements', { 'favoritesToApply': [{
        'elementId': targetWindow['elementId'],
        'favorite': 'WindowFavoriteFromPython'
    }] })

    detailsAfter = GetDetails (targetWindow)
    stayedInPlace = all (
        detailsBefore['typeSpecificDetails'][field] == detailsAfter['typeSpecificDetails'][field]
        for field in ['centerOffset', 'sillHeight', 'reflected', 'refSide', 'oSide'])
    print ('Window stayed in place: {}'.format (stayedInPlace))

    aclib.RunTapirCommand ('DeleteFavorites', { 'favorites': ['WindowFavoriteFromPython'] })
