import aclib

# Windows and doors carry a marker (the stamp drawn next to the opening on the floor plan)
# with a library part and GDL parameters of its own, separate from the opening's parameters
# that Get/SetGDLParametersOfElements reach. GetMarkerParametersOfElements returns the
# marker's library part and parameters, SetMarkerParametersOfElements changes the given
# marker parameters and leaves the opening's own settings as they are.

openings = []
for elementType in ['Window', 'Door']:
    openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']

if len (openings) == 0:
    print ('No windows or doors in the project.')
else:
    markerParametersOfElements = aclib.RunTapirCommand (
        'GetMarkerParametersOfElements', {'elements': openings})['markerParametersOfElements']

    # Sets the first simple (non-array) parameter of each marker to its current value, which
    # exercises the round trip without changing the project. A real script would set e.g. the
    # dimension prefix or the sill height reference of the marker here.
    elementsWithMarkerParameters = []
    for opening, markerParameters in zip (openings, markerParametersOfElements):
        if 'error' in markerParameters:
            continue
        simpleParameters = [
            p for p in markerParameters['parameters']
            if 'value' in p and 'dimension1' not in p and p['type'] in ('Integer', 'Length', 'RealNumber', 'Angle', 'String', 'Boolean')
        ]
        if len (simpleParameters) == 0:
            continue
        parameter = simpleParameters[0]
        elementsWithMarkerParameters.append ({
            'elementId': opening['elementId'],
            'markerParameters': [{
                'name': parameter['name'],
                'value': parameter['value']
            }]
        })

    if len (elementsWithMarkerParameters) > 0:
        aclib.RunTapirCommand (
            'SetMarkerParametersOfElements', {'elementsWithMarkerParameters': elementsWithMarkerParameters})
