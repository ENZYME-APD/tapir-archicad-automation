import aclib

newGDLParameterNameAndValue = {
    'name': 'gs_cont_pen',
    'value': 95
}

elements = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Object' })['elements']

aclib.RunTapirCommand ('SetGDLParametersOfElements', { 'elementsWithGDLParameters' : [{
        'elementId' : element['elementId'],
        'gdlParameters' : [newGDLParameterNameAndValue]
    } for element in elements] })

gdlParametersResponse = aclib.RunTapirCommand ('GetGDLParametersOfElements', { 'elements' : elements })
