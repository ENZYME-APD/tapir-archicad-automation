import aclib

gdlParameterName = 'gs_cont_pen'
newValue = 95

getElementsResponse = aclib.RunCommand ('GetElementsByType ', { "elementType": "Object" })
gdlParametersResponse = aclib.RunTapirCommand ('GetGDLParametersOfElements', getElementsResponse)
elements = getElementsResponse['elements']

changedGDLParameters = []
for i in range(len(elements)):
    print ('GDL Parameters of ' + elements[i]['elementId']['guid'])
    for name, details in gdlParametersResponse['gdlParametersOfElements'][i].items ():
        print ('\t' + name + ' = ' + str (details['value']))
        if 'name' == gdlParameterName:
            details['value'] = newValue
            changedGDLParameters.append({
                'elementId' : elements[i]['elementId'],
                'gdlParameters' : { name : details }
            })

aclib.RunTapirCommand ('SetGDLParametersOfElements', { 'elementsWithGDLParameters' : changedGDLParameters })

gdlParametersResponse = aclib.RunTapirCommand ('GetGDLParametersOfElements', getElementsResponse)
for i in range(len(elements)):
    elementGuid = str (elements[i]['elementId']['guid'])
    parameters = gdlParametersResponse['gdlParametersOfElements'][i]
    print ('gs_cont_pen of ' + elementGuid + ' after the change: ' + parameters['gs_cont_pen']['value'])