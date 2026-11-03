import aclib

# GetElementAtCoordinate finds the element of a polygonal type (Slab, Roof, Mesh, Zone,
# Hatch, ...) at a point of the floor plan, the way a click there would. A point inside each
# of the first few slabs is searched for a Slab, then a point far away from the building,
# where nothing is found.
slabs = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Slab'
    }, debug = False)['elements']

slabs = slabs[:5]

detailsOfSlabs = aclib.RunTapirCommand (
    'GetDetailsOfElements', {
        'elements': slabs
    }, debug = False)['detailsOfElements']

def GetPointInsideSlab (detailsOfSlab):
    # The average of the vertices of the first floor plan polygon is inside a convex slab;
    # without a floor plan polygon the average of the outline is used.
    floorPlanPolygons = detailsOfSlab.get ('floorPlanPolygons')
    if floorPlanPolygons:
        coordinates = floorPlanPolygons[0]['coordinates']
    else:
        coordinates = detailsOfSlab['details']['polygonOutline']
    return {
        'x': sum (c['x'] for c in coordinates) / len (coordinates),
        'y': sum (c['y'] for c in coordinates) / len (coordinates)
    }

for slab, detailsOfSlab in zip (slabs, detailsOfSlabs):
    response = aclib.RunTapirCommand (
        'GetElementAtCoordinate', {
            'elementType': 'Slab',
            'coordinate': GetPointInsideSlab (detailsOfSlab),
            'filters': ['IsVisibleByLayer']
        }, debug = False)
    if not response['found']:
        print ('Slab {}: no slab found at its point'.format (slab['elementId']['guid']))
    elif response['elementId']['guid'] == slab['elementId']['guid']:
        print ('Slab {}: found itself'.format (slab['elementId']['guid']))
    else:
        print ('Slab {}: found another slab {}'.format (slab['elementId']['guid'], response['elementId']['guid']))

# Far away from the building nothing is found.
response = aclib.RunTapirCommand (
    'GetElementAtCoordinate', {
        'elementType': 'Slab',
        'coordinate': {
            'x': 1000000.0,
            'y': 1000000.0
        }
    })
