import aclib

# GetElementConnections returns each pair of touching elements once, with the polygons
# of the surface where they touch, in world coordinates. Available from Archicad 28.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']
slabs = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Slab'
    }, debug = False)['elements']

response = aclib.RunTapirCommand (
    'GetElementConnections', {
        'elements': walls + slabs
    }, debug = False)

if 'error' in response:
    print ('Error:\n{}'.format (aclib.JsonDumpDictionary (response['error'])))
else:
    failed = [result for result in response['executionResults'] if not result['success']]
    print ('{} elements not found'.format (len (failed)))

    for connection in response['connections']:
        print ('{} - {}: {} polygons'.format (
            connection['elementId1']['guid'],
            connection['elementId2']['guid'],
            len (connection['polygons'])))
        for polygon in connection['polygons']:
            print ('  {} points'.format (len (polygon['coordinates'])))
