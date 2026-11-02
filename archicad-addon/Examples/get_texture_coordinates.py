import aclib

# GetTextureCoordinates returns, for each solid body of the 3D model of an element, the
# texture coordinates (u, v) of every face at the first vertex of the face, for example to
# map the textures of the surfaces when the geometry is exported.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

# The first few walls are enough to show the shape of the response.
walls = walls[:3]

textureCoordinates = aclib.RunTapirCommand (
    'GetTextureCoordinates', {
        'elements': walls
    }, debug = False)['textureCoordinates']

for wall, textureCoordinatesOfWall in zip (walls, textureCoordinates):
    print ('Wall {}'.format (wall['elementId']['guid']))
    if 'error' in textureCoordinatesOfWall:
        print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (textureCoordinatesOfWall['error'])))
        continue

    for body in textureCoordinatesOfWall['bodies']:
        if 'error' in body:
            print ('  Body without texture information')
            continue
        polygons = body['polygons']
        withCoordinates = [polygon for polygon in polygons if 'error' not in polygon]
        print ('  Body with {} faces, {} with texture coordinates'.format (len (polygons), len (withCoordinates)))
        for polygon in withCoordinates[:2]:
            print ('    u {:.3f}, v {:.3f}'.format (polygon['u'], polygon['v']))
