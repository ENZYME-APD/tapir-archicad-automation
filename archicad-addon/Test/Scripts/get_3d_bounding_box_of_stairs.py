import aclib

# Get3DBoundingBoxes reads the vertical extent of a Stair back from its 3D model, so the
# result of the stair solver can be checked against the requested totalHeight.
totalHeight = 3.25

createResult = aclib.RunTapirCommand (
    'CreateStairs', {
        'stairsData': [
            {
                'baseLinePoints': [
                    {'x': 0.0, 'y': 0.0},
                    {'x': 0.0, 'y': 5.0}
                ],
                'zCoordinate': 0.0,
                'totalHeight': totalHeight,
                'flightWidth': 1.2
            }
        ]
    })

stairs = [] if createResult is None else [
    {'elementId': e['elementId']} for e in createResult['elements'] if 'elementId' in e]
if not stairs:
    print ('Failed to create the stair.')
    raise SystemExit (1)

boundingBoxes = aclib.RunTapirCommand (
    'Get3DBoundingBoxes', {
        'elements': stairs
    }, debug = False)

# The boxes are printed rounded, because the exact extents depend on the stair structure
# of the currently loaded library.
for boundingBox in boundingBoxes['boundingBoxes3D']:
    if 'error' in boundingBox:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (boundingBox['error'])))
        continue
    box = boundingBox['boundingBox3D']
    height = box['zMax'] - box['zMin']
    print ('zMin = {0:.2f}, zMax = {1:.2f}'.format (box['zMin'], box['zMax']))
    print ('The stair spans {0:.2f} m vertically, totalHeight was {1:.2f} m'.format (height, totalHeight))
