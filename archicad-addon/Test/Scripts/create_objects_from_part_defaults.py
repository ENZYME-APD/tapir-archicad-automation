import aclib

# CreateObjects places a library part named by libraryPartName at its own default size and
# parameters, not at those of the part the Object tool holds, and takes GDL parameters and a layer
# on creation. The objects are deleted at the end, so the project is left as it was.

def Read (elementId):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['detailsOfElements'][0]
    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['gdlParametersOfElements'][0]['parameters']
    # GDL matches parameter names without case; a part's own list may spell ZZYZX as zzyzx.
    parametersByName = { parameter['name'].upper (): parameter.get ('value') for parameter in parameters }
    return details, parametersByName

def IsNear (value1, value2):
    return abs (value1 - value2) < 1e-4

layers = aclib.RunTapirCommand ('GetAttributesByType', { 'attributeType': 'Layer' }, debug = False)['attributes']
layerIndex = layers[0]['index']

elements = aclib.RunTapirCommand ('CreateObjects', {
    'objectsData': [
        { 'libraryPartName': 'Bed Double 01', 'coordinates': { 'x': 0.0, 'y': 30.0, 'z': 0.0 } },
        { 'libraryPartName': 'Bed Double 01', 'coordinates': { 'x': 3.0, 'y': 30.0, 'z': 0.0 },
          'parameters': [{ 'name': 'gs_bed_height', 'value': 0.5 }, { 'name': 'bHeadboard', 'value': False }],
          'layerIndex': layerIndex },
        { 'libraryPartName': 'Bed Double 01', 'coordinates': { 'x': 6.0, 'y': 30.0, 'z': 0.0 },
          'parameters': [{ 'name': 'no_such_parameter', 'value': 1 }] }
    ]
}, debug = False)['elements']

plain, withParameters, unknownName = elements

details, parameters = Read (plain['elementId'])
dimensions = details['details']['dimensions']
print ('Placed at its own size: dimensions agree with A and B:',
       IsNear (dimensions['x'], parameters['A']) and IsNear (dimensions['y'], parameters['B']))
print ('Its height is set:', dimensions['z'] > 0.0)

details, parameters = Read (withParameters['elementId'])
print ('GDL parameters set on creation:',
       IsNear (parameters['GS_BED_HEIGHT'], 0.5) and parameters['BHEADBOARD'] is False)
print ('Placed on the given layer:', details['layerIndex'] == layerIndex)

print ('An unknown parameter fails its item:', 'error' in unknownName)
print (unknownName.get ('error', {}).get ('message'))

aclib.RunTapirCommand ('DeleteElements', {
    'elements': [{ 'elementId': element['elementId'] } for element in elements if 'elementId' in element]
}, debug = False)
