import aclib
from collections import Counter

# GetDrawingPrimitivesOfElements needs the floor plan as the current database.
walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug=False)['elements']
if not walls:
    print ('No walls in the project.')
    raise SystemExit (0)

result = aclib.RunTapirCommand ('GetDrawingPrimitivesOfElements', {
    'elements': walls[:3]
}, debug=False)
if result is None or 'drawingPrimitivesOfElements' not in result:
    print ('Failed: {}'.format (result))
    raise SystemExit (1)

for wall, drawingPrimitives in zip (walls, result['drawingPrimitivesOfElements']):
    print ('-' * 30)
    print ('Wall {}'.format (wall['elementId']['guid']))
    if 'error' in drawingPrimitives:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (drawingPrimitives['error'])))
        continue
    primitives = drawingPrimitives['primitives']
    print ('Primitives by type: {}'.format (dict (Counter (p['type'] for p in primitives))))
    for primitive in primitives:
        if primitive['type'] == 'Line':
            print ('Line from ({0:.3f}, {1:.3f}) to ({2:.3f}, {3:.3f}), pen {4}'.format (
                primitive['c1']['x'], primitive['c1']['y'], primitive['c2']['x'], primitive['c2']['y'], primitive['pen']))
        elif primitive['type'] == 'Poly':
            for polygon in primitive.get ('polygons', []):
                print ('Polygon with {} vertices'.format (len (polygon['coordinates'])))
