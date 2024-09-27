import aclib

gdlParameterName = 'gs_cont_pen'
newValue = 95

getElementsResponse = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Object' })
gdlParametersResponse = aclib.RunTapirCommand ('GetGDLParametersOfElements', getElementsResponse)
elements = getElementsResponse['elements']

changedGDLParameters = []
for i in range(len(elements)):
    for elemGdlParameters in gdlParametersResponse['gdlParametersOfElements']:
        for details in elemGdlParameters['parameters']:
            if details['name'] == gdlParameterName:
                details['value'] = newValue
                changedGDLParameters.append({
                    'elementId' : elements[i]['elementId'],
                    'gdlParameters' : [
                        {
                            'parameters' : [
                                details
                            ]
                        }
                    ]
                })

aclib.RunTapirCommand ('SetGDLParametersOfElements', { 'elementsWithGDLParameters' : changedGDLParameters })

gdlParametersResponse = aclib.RunTapirCommand ('GetGDLParametersOfElements', getElementsResponse)
