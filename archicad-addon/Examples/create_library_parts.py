import aclib

# CreateLibraryParts writes a GDL object into the embedded library from its scripts and
# parameter definitions, so a script can generate parametric objects without the GDL editor.
# This creates a box whose height is a parameter, places it with CreateObjects, reads the
# parameter back, then deletes the placed box and the library part, so the project is left
# as it was (the deletion needs Archicad 27 or newer).

libraryPartName = 'Tapir Example Box'

# The parent subtype by its GUID, found among the loaded library parts: the first of these
# subtype templates of the Archicad Library that is loaded.
subtypeNames = ['General Object', 'Model Element', 'General GDL Object']
availableLibraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {
    'filterByTypeId': 'Object'
}, debug = False)['libraryParts']
subtypeGuids = { part['documentName']: part['guid'] for part in availableLibraryParts }
parentGuid = next ((subtypeGuids[name] for name in subtypeNames if name in subtypeGuids), None)
print ('Parent subtype found:', parentGuid is not None)

# Without the GUID the parent is given by its document name instead.
parent = { 'parentId': parentGuid } if parentGuid is not None else { 'parentName': subtypeNames[0] }

response = aclib.RunTapirCommand ('CreateLibraryParts', {
    'libraryParts': [{
        'name': libraryPartName,
        **parent,
        'path': 'TapirExamples/{}.gsm'.format (libraryPartName),
        'a': 0.6,
        'b': 0.4,
        'parameters': [
            { 'name': 'boxHeight', 'type': 'Length', 'value': 0.5, 'description': 'Box Height' },
            { 'name': 'boxSurface', 'type': 'Material', 'value': 1, 'description': 'Box Surface' },
            { 'name': 'boxLabel', 'type': 'String', 'value': 'Tapir', 'description': 'Label' },
            { 'name': 'boxId', 'type': 'Integer', 'value': 1, 'isHidden': True }
        ],
        'masterScript': 'boxVolume = A * B * boxHeight',
        'script2D': 'RECT2 0, 0, A, B\nTEXT2 A / 2, B / 2, boxLabel',
        'script3D': 'MATERIAL boxSurface\nBLOCK A, B, boxHeight',
        'parameterScript': 'VALUES "boxHeight" RANGE [0.1, 3.0]'
    }],
    'overwriteExisting': True
}, debug = False)
createdPart = response['libraryParts'][0]
print ('Library part created:', 'error' not in createdPart)
if 'error' in createdPart:
    print (createdPart['error']['message'])
else:
    # The new part is loaded, so CreateObjects places it by name right away.
    elementId = aclib.RunTapirCommand ('CreateObjects', {
        'objectsData': [{
            'libraryPartName': libraryPartName,
            'coordinates': { 'x': 0.0, 'y': 30.0, 'z': 0.0 }
        }]
    }, debug = False)['elements'][0]['elementId']

    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['gdlParametersOfElements'][0]['parameters']
    parametersByName = { parameter['name']: parameter for parameter in parameters }
    print ('boxHeight parameter:', parametersByName['boxHeight']['value'])
    print ('boxLabel parameter:', parametersByName['boxLabel']['value'])

    aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] }, debug = False)

    # Needs Archicad 27 or newer; before that the command answers with an error.
    response = aclib.RunTapirCommand ('DeleteEmbeddedLibraryItems', {
        'embeddedLibraryItems': [{ 'path': createdPart['path'] }]
    }, debug = False)
    print ('Library part deleted:', 'executionResults' in response and response['executionResults'][0]['success'])
