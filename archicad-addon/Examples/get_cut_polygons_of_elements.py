import aclib

# GetCutPolygonsOfElements cuts each body of the 3D model of the elements with a plane and
# returns its building material, the cut area and the cut polygons in 2D coordinates of the
# plane. A composite structure has one body per skin. Available from Archicad 28.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

# A horizontal plane 1 m above the project zero; its 2D coordinates are the floor plan's.
response = aclib.RunTapirCommand (
    'GetCutPolygonsOfElements', {
        'elements': walls,
        'cutPlane': {
            'basePoint': {'x': 0.0, 'y': 0.0, 'z': 1.0},
            'axisX': {'x': 1.0, 'y': 0.0, 'z': 0.0},
            'axisY': {'x': 0.0, 'y': 1.0, 'z': 0.0},
            'axisZ': {'x': 0.0, 'y': 0.0, 'z': 1.0}
        }
    }, debug = False)

if 'error' in response:
    print ('Error:\n{}'.format (aclib.JsonDumpDictionary (response['error'])))
else:
    for wall, result in zip (walls, response['cutPolygons']):
        if 'error' in result:
            print ('{}: error {}'.format (wall['elementId']['guid'], result['error']['message']))
            continue

        print ('{}: {} bodies'.format (wall['elementId']['guid'], len (result['bodies'])))
        for body in result['bodies']:
            print ('  {}: {}, area {:.3f}, {} polygons'.format (
                body['bodyIndex'],
                body.get ('buildingMaterialName', '-'),
                body['area'],
                len (body['polygons'])))
            for polygon in body['polygons']:
                print ('    {} points, {} holes'.format (len (polygon['polygonOutline']), len (polygon['holes'])))
