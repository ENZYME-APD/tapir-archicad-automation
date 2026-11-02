import aclib

# GetSurfaceTypesOfElements returns the surface type (the numeric API_SurfaceTypeID value) of every
# polygon of the 3D model of the elements, for example to tell the sides of a Wall or a Roof apart.
for elementType in ['Wall', 'Slab', 'Roof']:
    elements = aclib.RunTapirCommand (
        'GetElementsByType', {
            'elementType': elementType
        }, debug = False)['elements']

    if not elements:
        print ('The project has no {}s.'.format (elementType))
        continue

    surfaceTypes = aclib.RunTapirCommand (
        'GetSurfaceTypesOfElements', {
            'elements': elements[:3]
        }, debug = False)['surfaceTypes']

    for element, result in zip (elements, surfaceTypes):
        print ('{} {}'.format (elementType, element['elementId']['guid']))
        if 'error' in result:
            print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (result['error'])))
            continue
        polygonsPerType = {}
        for polygon in result['polygons']:
            polygonsPerType[polygon['surfaceType']] = polygonsPerType.get (polygon['surfaceType'], 0) + 1
        print ('  {} polygons, per surface type: {}'.format (len (result['polygons']), polygonsPerType))
