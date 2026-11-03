import aclib

# TriangulatePolygonsOfElements returns the triangles of the plan polygon of Slabs, Zones, Hatches,
# Meshes and Roofs, for example for renderers and exporters to web viewers.
for elementType in ['Slab', 'Zone', 'Hatch', 'Mesh', 'Roof']:
    elements = aclib.RunTapirCommand (
        'GetElementsByType', {
            'elementType': elementType
        }, debug = False)['elements']

    if not elements:
        print ('The project has no {}s.'.format (elementType))
        continue

    triangulations = aclib.RunTapirCommand (
        'TriangulatePolygonsOfElements', {
            'elements': elements
        }, debug = False)['triangulations']

    for element, triangulation in zip (elements, triangulations):
        print ('{} {}'.format (elementType, element['elementId']['guid']))
        if 'error' in triangulation:
            print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (triangulation['error'])))
            continue
        triangles = triangulation['triangles']
        area = 0.0
        for triangle in triangles:
            a, b, c = triangle['vertices']
            area += abs ((b['x'] - a['x']) * (c['y'] - a['y']) - (c['x'] - a['x']) * (b['y'] - a['y'])) / 2.0
        print ('  {} triangles, area {:.3f}'.format (len (triangles), area))

# An element of another type gets an error item.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

if walls:
    result = aclib.RunTapirCommand (
        'TriangulatePolygonsOfElements', {
            'elements': walls[:1]
        }, debug = False)['triangulations']
    print ('A Wall gets an error: {}'.format ('error' in result[0]))

    # With convex: true the faces of the 3D model of any element are decomposed into convex
    # polygons, given by the 3D coordinates of their vertices.
    result = aclib.RunTapirCommand (
        'TriangulatePolygonsOfElements', {
            'elements': walls[:1],
            'convex': True
        }, debug = False)['triangulations']
    if 'error' in result[0]:
        print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (result[0]['error'])))
    else:
        convexPolygons = result[0]['convexPolygons']
        print ('A Wall decomposed into {} convex polygons, the smallest with {} vertices'.format (
            len (convexPolygons), min ((len (polygon['vertices']) for polygon in convexPolygons), default = 0)))
