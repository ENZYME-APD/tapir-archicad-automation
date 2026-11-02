import aclib

# GetSkylightHolePolygons returns the hole each Skylight cuts into its host Roof or Shell,
# for example for daylight or roof area calculations.
skylights = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Skylight'
    }, debug = False)['elements']

if not skylights:
    print ('The project has no Skylights.')
else:
    holePolygons = aclib.RunTapirCommand (
        'GetSkylightHolePolygons', {
            'elements': skylights
        }, debug = False)['skylightHolePolygons']

    for skylight, holePolygon in zip (skylights, holePolygons):
        print ('Skylight {}'.format (skylight['elementId']['guid']))
        if 'error' in holePolygon:
            print ('  Error:\n{}'.format (aclib.JsonDumpDictionary (holePolygon['error'])))
            continue
        outline = holePolygon['polygonOutline']
        print ('  Hole outline with {} vertices and {} holes'.format (len (outline), len (holePolygon['holes'])))
        for coordinate in outline:
            print ('    ({:.3f}, {:.3f})'.format (coordinate['x'], coordinate['y']))

# An element that is not a Skylight gets an error item.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

if walls:
    result = aclib.RunTapirCommand (
        'GetSkylightHolePolygons', {
            'elements': walls[:1]
        }, debug = False)['skylightHolePolygons']
    print ('A Wall gets an error: {}'.format ('error' in result[0]))
