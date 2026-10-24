import aclib

# A stair, built here so the example runs on any project, read back through
# GetDetailsOfElements: the number of risers, the riser height, the tread depth, the flight
# width, the total height, the base elevation and the baseline. The 3D bounding boxes of its
# risers, treads and structures are read too (Get3DBoundingBoxes reads a Stair and its
# subelements from their 3D bodies). The stair is deleted again at the end.

totalHeight = 3.0
flightWidth = 1.2
baseLinePoints = [{'x': 100.0, 'y': 100.0}, {'x': 100.0, 'y': 105.0}]

createResult = aclib.RunTapirCommand ('CreateStairs', {'stairsData': [
    {
        'baseLinePoints': baseLinePoints,
        'zCoordinate': 0.0,
        'totalHeight': totalHeight,
        'flightWidth': flightWidth
    }
]}, debug = False)

stairs = [] if createResult is None else [
    {'elementId': e['elementId']} for e in createResult['elements'] if 'elementId' in e]
if not stairs:
    print ('Failed to create the stair.')
    raise SystemExit (1)

detailsOfStairs = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': stairs,
    'fields': ['type', 'details']
}, debug = False)['detailsOfElements']

# The values are printed rounded, because the stair solver adjusts them to the stair
# structure of the currently loaded library.
for detailsOfStair in detailsOfStairs:
    if 'error' in detailsOfStair:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (detailsOfStair['error'])))
        continue
    details = detailsOfStair['details']
    print ('type = {}'.format (detailsOfStair['type']))
    print ('stepNum = {}'.format (details['stepNum']))
    print ('riserHeight = {0:.2f}, treadDepth = {1:.2f}'.format (details['riserHeight'], details['treadDepth']))
    print ('flightWidth = {0:.2f}, totalHeight = {1:.2f}'.format (details['flightWidth'], details['totalHeight']))
    print ('level = {0:.2f}, zCoordinate = {1:.2f}'.format (details['level'], details['zCoordinate']))
    print ('finishVisible = {}'.format (details['finishVisible']))
    print ('baseLinePoints = {}'.format (
        ', '.join ('({0:.2f}, {1:.2f})'.format (p['x'], p['y']) for p in details['baseLinePoints'])))
    print ('stepNum * riserHeight = {0:.2f}, totalHeight was {1:.2f}'.format (
        details['stepNum'] * details['riserHeight'], totalHeight))

subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': stairs}, debug = False)['subelements']
for subelementsOfStair in subelements:
    for subelementType in ['stairRisers', 'stairTreads', 'stairStructures']:
        subelementsOfType = subelementsOfStair.get (subelementType, [])
        if not subelementsOfType:
            print ('{}: none'.format (subelementType))
            continue
        boundingBoxes = aclib.RunTapirCommand ('Get3DBoundingBoxes', {'elements': subelementsOfType}, debug = False)['boundingBoxes3D']
        errors = [b for b in boundingBoxes if 'error' in b]
        boxes = [b['boundingBox3D'] for b in boundingBoxes if 'boundingBox3D' in b]
        print ('{0}: {1} subelements, {2} bounding boxes, {3} errors'.format (
            subelementType, len (subelementsOfType), len (boxes), len (errors)))
        if boxes:
            zMin = min (b['zMin'] for b in boxes)
            zMax = max (b['zMax'] for b in boxes)
            print ('  together they span z = {0:.2f} .. {1:.2f}'.format (zMin, zMax))
        for error in errors:
            print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (error['error'])))

aclib.RunTapirCommand ('DeleteElements', {'elements': stairs}, debug = False)
