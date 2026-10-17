import aclib

# GetElementAtCoordinate finds the element of a type at a point of the floor plan, the way
# a click there would. A point inside each of the first few walls is searched for a Wall,
# then a point far away from the building, where nothing is found.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

walls = walls[:5]

detailsOfWalls = aclib.RunTapirCommand (
    'GetDetailsOfElements', {
        'elements': walls
    }, debug = False)['detailsOfElements']

def GetPointInsideWall (detailsOfWall):
    # The average of the vertices of the first floor plan polygon is inside a straight wall;
    # without a floor plan polygon the midpoint of the reference line is used.
    floorPlanPolygons = detailsOfWall.get ('floorPlanPolygons')
    if floorPlanPolygons:
        coordinates = floorPlanPolygons[0]['coordinates']
        return {
            'x': sum (c['x'] for c in coordinates) / len (coordinates),
            'y': sum (c['y'] for c in coordinates) / len (coordinates)
        }
    beg = detailsOfWall['details']['begCoordinate']
    end = detailsOfWall['details']['endCoordinate']
    return {
        'x': (beg['x'] + end['x']) / 2,
        'y': (beg['y'] + end['y']) / 2
    }

for wall, detailsOfWall in zip (walls, detailsOfWalls):
    response = aclib.RunTapirCommand (
        'GetElementAtCoordinate', {
            'elementType': 'Wall',
            'coordinate': GetPointInsideWall (detailsOfWall),
            'filters': ['IsVisibleByLayer']
        }, debug = False)
    if not response['found']:
        print ('Wall {}: no wall found at its point'.format (wall['elementId']['guid']))
    elif response['elementId']['guid'] == wall['elementId']['guid']:
        print ('Wall {}: found itself'.format (wall['elementId']['guid']))
    else:
        print ('Wall {}: found another wall {}'.format (wall['elementId']['guid'], response['elementId']['guid']))

# Far away from the building nothing is found.
response = aclib.RunTapirCommand (
    'GetElementAtCoordinate', {
        'elementType': 'Wall',
        'coordinate': {
            'x': 1000000.0,
            'y': 1000000.0
        }
    })
