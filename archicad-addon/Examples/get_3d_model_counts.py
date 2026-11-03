import aclib

# Get3DModelCounts returns how many components of each type (bodies, polygons, polygon
# edges, edges, vertices, normal vectors, materials, lights) the 3D model Archicad
# currently holds. Available from Archicad 27.
counts = aclib.RunTapirCommand (
    'Get3DModelCounts', {}, debug = False)

if 'error' in counts:
    print ('Error:\n{}'.format (aclib.JsonDumpDictionary (counts['error'])))
else:
    for item in counts['counts']:
        if 'error' in item:
            print ('Error:\n{}'.format (aclib.JsonDumpDictionary (item['error'])))
            continue
        print ('{}: {}'.format (item['componentType'], item['count']))
