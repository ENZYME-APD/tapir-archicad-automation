import aclib

# GetDetailsOfElements must report an object's 'dimensions' in metres: the same size as its GDL
# parameters A and B. An object placed without a fixed size (useFixSize off) stores its size
# divided by the library part's own A and B, so both settings are checked. Chair 01 keeps A and B
# equal, so the size is square. The chairs are deleted at the end, so the project is left as it was.

def IsNear (value1, value2):
    return abs (value1 - value2) < 1e-4

for useFixSize in (False, True):
    print ('useFixSize: {}'.format (useFixSize))
    elementId = aclib.RunTapirCommand ('CreateObjects', {
        'objectsData': [{
            'libraryPartName': 'Chair 01',
            'coordinates': { 'x': 0.0, 'y': 30.0, 'z': 0.0 },
            'dimensions': { 'x': 0.55, 'y': 0.55, 'z': 0.85 },
            'useFixSize': useFixSize
        }]
    }, debug = False)['elements'][0]['elementId']

    dimensions = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['detailsOfElements'][0]['details']['dimensions']
    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['gdlParametersOfElements'][0]['parameters']
    parametersByName = { parameter['name']: parameter['value'] for parameter in parameters }
    print ('  dimensions x and y equal A and B:',
           IsNear (dimensions['x'], parametersByName['A']) and IsNear (dimensions['y'], parametersByName['B']))

    aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] }, debug = False)
