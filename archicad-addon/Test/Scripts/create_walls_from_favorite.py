"""
Example for issue #766: CreateWalls with a 'favoriteName' takes the wall's structure
(composite), reference line location, top link and offsets from the favorite, so a wall
can be created from a favorite by giving only its reference line endpoints. Explicitly
given fields (floorIndex, height, thickness, ...) override the favorite.

The script builds its own composite, top-linked wall, saves it as a favorite, then
creates two walls from that favorite - one with only the endpoints, one with an explicit
floorIndex and height - reads them back, prints what matched, and cleans up after itself.
"""
import aclib

def run (command, parameters = None):
    return aclib.RunTapirCommand (command, parameters or {}, debug = False)

passes = fails = 0

def check (label, expected, got):
    global passes, fails
    ok = (got == expected)
    if ok:
        passes += 1
    else:
        fails += 1
    print ('  [{}] {}  (expected={!r}, got={!r})'.format ('PASS' if ok else 'FAIL', label, expected, got))

def wallDetails (elementId):
    return run ('GetDetailsOfElements', {'elements': [{'elementId': elementId}]})['detailsOfElements'][0]['details']

favoriteName = 'CompositeWallFromPython'
created = []

composites = run ('GetAttributesByType', {'attributeType': 'Composite'})['attributes']
if len (composites) == 0:
    print ('No composite attribute in the project; nothing to build the favorite from.')
else:
    compositeId = composites[0]['attributeId']

    # --- the template wall the favorite is saved from --------------------------------
    template = run ('CreateWalls', {'wallsData': [{
        'begCoordinate': {'x': 5000.0, 'y': 0.0},
        'endCoordinate': {'x': 5005.0, 'y': 0.0},
        'height': 3.0,
        'thickness': 0.25,
        'structureType': 'Composite',
        'compositeId': compositeId,
        'referenceLineLocation': 'CoreOutside',
        'offset': 0.1
    }]})['elements'][0]['elementId']
    created.append (template)
    # Link the top to the next story; CreateWalls itself has no relativeTopStory field.
    run ('ModifyWalls', {'wallsWithDetails': [{'elementId': template, 'relativeTopStory': 1}]})
    templateDetails = wallDetails (template)
    print ('Template wall: structureType={}, referenceLineLocation={}, relativeTopStory={}, offset={}'.format (
        templateDetails['structureType'], templateDetails['referenceLineLocation'],
        templateDetails['relativeTopStory'], templateDetails['offset']))

    run ('CreateFavoritesFromElements', {'favoritesFromElements': [{
        'elementId': template,
        'favorite': favoriteName
    }]})

    # --- 1. only the endpoints: everything else comes from the favorite --------------
    print ('CreateWalls with favoriteName and the endpoints only')
    response = run ('CreateWalls', {'wallsData': [{
        'begCoordinate': {'x': 5000.0, 'y': 5.0},
        'endCoordinate': {'x': 5005.0, 'y': 5.0},
        'favoriteName': favoriteName
    }]})
    check ('wall created', True, 'elementId' in response['elements'][0])
    if 'elementId' in response['elements'][0]:
        created.append (response['elements'][0]['elementId'])
        d = wallDetails (response['elements'][0]['elementId'])
        check ('structureType from the favorite', templateDetails['structureType'], d['structureType'])
        check ('compositeId from the favorite', templateDetails.get ('compositeId'), d.get ('compositeId'))
        check ('referenceLineLocation from the favorite', templateDetails['referenceLineLocation'], d['referenceLineLocation'])
        check ('relativeTopStory from the favorite', templateDetails['relativeTopStory'], d['relativeTopStory'])
        check ('offset from the favorite', templateDetails['offset'], d['offset'])
        check ('thickness from the favorite', templateDetails['begThickness'], d['begThickness'])

    # --- 2. favoriteName with floorIndex and height: the given fields win ------------
    print ('CreateWalls with favoriteName, floorIndex and an explicit height')
    response = run ('CreateWalls', {'wallsData': [{
        'begCoordinate': {'x': 5000.0, 'y': 10.0},
        'endCoordinate': {'x': 5005.0, 'y': 10.0},
        'favoriteName': favoriteName,
        'floorIndex': 0,
        'height': 1.0
    }]})
    check ('wall created', True, 'elementId' in response['elements'][0])
    if 'elementId' in response['elements'][0]:
        created.append (response['elements'][0]['elementId'])
        d = wallDetails (response['elements'][0]['elementId'])
        check ('structureType from the favorite', templateDetails['structureType'], d['structureType'])
        check ('referenceLineLocation from the favorite', templateDetails['referenceLineLocation'], d['referenceLineLocation'])
        check ('explicit height replaces the top link', 0, d['relativeTopStory'])
        check ('explicit height applied', 1.0, d['height'])

    # --- 3. an unknown favorite is reported by name -----------------------------------
    print ('CreateWalls with a favoriteName that does not exist')
    response = run ('CreateWalls', {'wallsData': [{
        'begCoordinate': {'x': 5000.0, 'y': 15.0},
        'endCoordinate': {'x': 5005.0, 'y': 15.0},
        'favoriteName': favoriteName + ' (no such favorite)'
    }]})
    error = response['elements'][0].get ('error', {})
    check ('error names the favorite', True, favoriteName in error.get ('message', ''))

    # --- cleanup -----------------------------------------------------------------------
    run ('DeleteFavorites', {'favorites': [favoriteName]})
    run ('DeleteElements', {'elements': [{'elementId': elementId} for elementId in created]})

    print ('{} passed, {} failed'.format (passes, fails))
