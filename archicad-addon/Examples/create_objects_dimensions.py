import aclib

# CreateObjects and ModifyObjects must give an object exactly the 'dimensions' asked for. An object
# placed without a fixed size (useFixSize off) stores its size divided by the library part's own A
# and B, so both settings are checked. Chair 01 keeps A and B equal, so the sizes are square.
# The chairs are deleted at the end, so the project is left as it was.

def GetAB (elementId):
    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['gdlParametersOfElements'][0]['parameters']
    parametersByName = { parameter['name']: parameter['value'] for parameter in parameters }
    return parametersByName['A'], parametersByName['B']

def IsNear (value1, value2):
    return abs (value1 - value2) < 1e-4

for useFixSize in (False, True):
    print ('useFixSize: {}'.format (useFixSize))
    elementId = aclib.RunTapirCommand ('CreateObjects', {
        'objectsData': [{
            'libraryPartName': 'Chair 01',
            'coordinates': { 'x': 0.0, 'y': 25.0, 'z': 0.0 },
            'dimensions': { 'x': 0.55, 'y': 0.55, 'z': 0.85 },
            'useFixSize': useFixSize
        }]
    }, debug = False)['elements'][0]['elementId']
    a, b = GetAB (elementId)
    print ('  CreateObjects places A and B as given:', IsNear (a, 0.55) and IsNear (b, 0.55))

    aclib.RunTapirCommand ('ModifyObjects', {
        'objectsWithDetails': [{
            'elementId': elementId,
            'dimensions': { 'x': 0.65, 'y': 0.65, 'z': 0.9 }
        }]
    }, debug = False)
    a, b = GetAB (elementId)
    print ('  ModifyObjects sets A and B as given:', IsNear (a, 0.65) and IsNear (b, 0.65))

    aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] }, debug = False)
