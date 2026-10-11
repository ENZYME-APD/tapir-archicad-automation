import aclib

# GetGDLParametersOfElements returns an array parameter's value as a flat list, row by row,
# together with its dimensions (dimension1 rows of dimension2 items). The same flat list can be
# written back with SetGDLParametersOfElements: for a two-dimensional array it is read row by row
# with the array's current number of columns, so a Get -> Set round trip keeps the array's shape
# (#812: before, the flat list turned a 4 x 2 array into an 8 x 1 one, which broke the object).
# A flat list whose length is not a multiple of dimension2 is refused.

elements = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Object' })['elements']

gdlParametersOfElements = aclib.RunTapirCommand (
    'GetGDLParametersOfElements', { 'elements': elements }, debug = False)['gdlParametersOfElements']

elementWith2DArray = None
for element, gdlParameters in zip (elements, gdlParametersOfElements):
    for parameter in gdlParameters.get ('parameters', []):
        if parameter.get ('dimension2', 1) > 1 and 'value' in parameter and not parameter['isLocked']:
            elementWith2DArray = (element, parameter)
            break
    if elementWith2DArray is not None:
        break

if elementWith2DArray is None:
    print ('No object with a two-dimensional array parameter in the project.')
else:
    element, parameter = elementWith2DArray
    flatValues = parameter['value']

    # The value exactly as it was read back: dimension1 x dimension2 stays the same.
    aclib.RunTapirCommand ('SetGDLParametersOfElements', { 'elementsWithGDLParameters': [{
        'elementId': element['elementId'],
        'gdlParameters': [{ 'name': parameter['name'], 'value': flatValues }]
    }] })

    # One item too many for whole rows: refused, the parameter is left as it is.
    aclib.RunTapirCommand ('SetGDLParametersOfElements', { 'elementsWithGDLParameters': [{
        'elementId': element['elementId'],
        'gdlParameters': [{ 'name': parameter['name'], 'value': flatValues + [flatValues[0]] }]
    }] })

    parameterAfter = next (
        p for p in aclib.RunTapirCommand ('GetGDLParametersOfElements', { 'elements': [element] }, debug = False)
            ['gdlParametersOfElements'][0]['parameters']
        if p['name'] == parameter['name'])
    print ('Dimensions kept: {}'.format (
        (parameterAfter['dimension1'], parameterAfter['dimension2']) == (parameter['dimension1'], parameter['dimension2'])))
