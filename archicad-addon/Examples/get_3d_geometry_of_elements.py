import aclib

# Get3DGeometryOfElements returns the solid bodies of the 3D model of each element:
# the vertices in world coordinates and the faces as loops of vertex indices (with
# their holes), so the geometry can be rebuilt as a mesh elsewhere.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

# The first few walls are enough to show the shape of the response.
walls = walls[:3]

geometries = aclib.RunTapirCommand (
    'Get3DGeometryOfElements', {
        'elements': walls
    }, debug = False)['geometries3D']

for geometry in geometries:
    if 'error' in geometry:
        print ('Error:\n{}'.format (aclib.JsonDumpDictionary (geometry['error'])))
        continue

    print ('Wall with {} solid bodies'.format (len (geometry['bodies'])))
    for body in geometry['bodies']:
        vertices = body['vertices']
        polygons = body['polygons']
        facesWithHoles = sum (1 for polygon in polygons if 'holes' in polygon)
        facesWithSurface = sum (1 for polygon in polygons if 'surfaceId' in polygon)
        print ('  {} vertices, {} faces ({} with holes, {} with a surface attribute)'.format (
            len (vertices), len (polygons), facesWithHoles, facesWithSurface))
        # The extents are printed rounded, because the exact vertices depend on the wall
        # junctions and openings Archicad resolves in the 3D model.
        for axis in ['x', 'y', 'z']:
            values = [vertex[axis] for vertex in vertices]
            print ('  {}: {:.2f} .. {:.2f}'.format (axis, min (values), max (values)))
