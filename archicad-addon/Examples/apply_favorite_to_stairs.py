import aclib

# Applying a Favorite must never move the target element - not even for a Stair, whose
# geometry lives in the element memo the Favorite carries around (#576). The two stairs
# below are created at known, different baselines; after the first one's Favorite is
# applied to the second, the second stair must still be where it was created.

createdStairs = aclib.RunTapirCommand ('CreateStairs', {
    'stairsData': [
        {
            'baseLinePoints': [
                { 'x': 80.0, 'y': 0.0 },
                { 'x': 84.0, 'y': 0.0 }
            ],
            'zCoordinate': 0.0
        },
        {
            'baseLinePoints': [
                { 'x': 80.0, 'y': 10.0 },
                { 'x': 84.0, 'y': 10.0 }
            ],
            'zCoordinate': 0.0
        }
    ]
})['elements']

sourceStair = createdStairs[0]
targetStair = createdStairs[1]

# The exact box depends on the project's stair defaults, so only the verdict is printed.
def GetBoundingBoxOf (element):
    boundingBoxes = aclib.RunTapirCommand ('Get3DBoundingBoxes', {
        'elements': [element]
    }, debug = False)['boundingBoxes3D']
    return boundingBoxes[0]['boundingBox3D']

boundingBoxBefore = GetBoundingBoxOf (targetStair)

aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{
        'elementId': sourceStair['elementId'],
        'favorite': 'StairFromPython'
    }]
})

aclib.RunTapirCommand ('ApplyFavoritesToElements', {
    'favoritesToApply': [{
        'elementId': targetStair['elementId'],
        'favorite': 'StairFromPython'
    }]
})

boundingBoxAfter = GetBoundingBoxOf (targetStair)

hasMoved = any (
    abs (boundingBoxBefore[field] - boundingBoxAfter[field]) > 1e-6
    for field in ['xMin', 'xMax', 'yMin', 'yMax'])
print ('Stair moved by applying the favorite: {}'.format (hasMoved))

aclib.RunTapirCommand ('DeleteFavorites', {
    'favorites': ['StairFromPython']
})
