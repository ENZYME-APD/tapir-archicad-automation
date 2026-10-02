import aclib

# SetGDLParametersOfElements must leave a placed object's size alone when A and B are not written,
# and must give it exactly the A and B that are written. An object placed without a fixed size
# (useFixSize off) stores its size divided by the library part's own A and B, so both settings are
# checked. The chairs are deleted at the end, so the project is left as it was.

def GetSize (elementId):
    parameters = aclib.RunTapirCommand ('GetGDLParametersOfElements', {
        'elements': [{ 'elementId': elementId }]
    }, debug = False)['gdlParametersOfElements'][0]['parameters']
    parametersByName = { parameter['name']: parameter for parameter in parameters }
    box = aclib.RunCommand ('API.Get3DBoundingBoxes', {
        'elements': [{ 'elementId': elementId }]
    })['boundingBoxes3D'][0]['boundingBox3D']
    size = (parametersByName['A']['value'], parametersByName['B']['value'],
            box['xMax'] - box['xMin'], box['yMax'] - box['yMin'])
    return parametersByName, size

def SetGDLParameters (elementId, gdlParameters):
    return aclib.RunTapirCommand ('SetGDLParametersOfElements', {
        'elementsWithGDLParameters': [{ 'elementId': elementId, 'gdlParameters': gdlParameters }]
    }, debug = False)['executionResults'][0]

def IsNear (value1, value2):
    return abs (value1 - value2) < 1e-4

for useFixSize in (False, True):
    print ('useFixSize: {}'.format (useFixSize))
    elementId = aclib.RunTapirCommand ('CreateObjects', {
        'objectsData': [
            { 'libraryPartName': 'Chair 01', 'coordinates': { 'x': 0.0, 'y': 20.0, 'z': 0.0 }, 'useFixSize': useFixSize }
        ]
    }, debug = False)['elements'][0]['elementId']

    parametersByName, _ = GetSize (elementId)
    unrelatedParameter = next (parameter for parameter in parametersByName.values ()
                               if parameter['type'] == 'Integer' and 'dimension1' not in parameter
                               and not parameter['isLocked'])
    unrelatedWrite = [{ 'name': unrelatedParameter['name'], 'value': unrelatedParameter['value'] }]

    # The chair's Parameter Script keeps A and B equal, which it may have done at placement without
    # the placed size following. The first write brings the placed size in line with A and B, as an
    # edit in Archicad does.
    SetGDLParameters (elementId, unrelatedWrite)
    _, placedSize = GetSize (elementId)

    # An unrelated parameter written back with its own value, twice more: the size stays.
    for _ in range (2):
        SetGDLParameters (elementId, unrelatedWrite)
    _, sizeAfterUnrelatedWrites = GetSize (elementId)
    print ('  Unrelated writes keep A, B and the 3D size:',
           all (IsNear (before, after) for before, after in zip (placedSize, sizeAfterUnrelatedWrites)))

    # A and B written: they read back as written.
    newA = placedSize[0] + 0.1
    newB = placedSize[1] + 0.1
    SetGDLParameters (elementId, [{ 'name': 'A', 'value': newA }, { 'name': 'B', 'value': newB }])
    _, sizeAfterSizeWrite = GetSize (elementId)
    print ('  Written A and B read back as written:',
           IsNear (sizeAfterSizeWrite[0], newA) and IsNear (sizeAfterSizeWrite[1], newB))

    # Writing A and B again with the same values changes nothing.
    SetGDLParameters (elementId, [{ 'name': 'A', 'value': newA }, { 'name': 'B', 'value': newB }])
    _, sizeAfterSecondSizeWrite = GetSize (elementId)
    print ('  A second identical write keeps the size:',
           all (IsNear (first, second) for first, second in zip (sizeAfterSizeWrite, sizeAfterSecondSizeWrite)))

    aclib.RunTapirCommand ('DeleteElements', { 'elements': [{ 'elementId': elementId }] }, debug = False)
