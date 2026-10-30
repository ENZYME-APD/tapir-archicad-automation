import aclib

# GetBodyMaterialsOfElements returns the building material of each body of the 3D model
# of the elements; a composite structure has one body per skin. Available from Archicad 28.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    }, debug = False)['elements']

response = aclib.RunTapirCommand (
    'GetBodyMaterialsOfElements', {
        'elements': walls
    }, debug = False)

if 'error' in response:
    print ('Error:\n{}'.format (aclib.JsonDumpDictionary (response['error'])))
else:
    for wall, result in zip (walls, response['bodyMaterials']):
        if 'error' in result:
            print ('{}: error {}'.format (wall['elementId']['guid'], result['error']['message']))
            continue

        print ('{}: {} bodies'.format (wall['elementId']['guid'], len (result['bodies'])))
        for body in result['bodies']:
            print ('  {}: {}'.format (body['bodyIndex'], body.get ('buildingMaterialName', '-')))
