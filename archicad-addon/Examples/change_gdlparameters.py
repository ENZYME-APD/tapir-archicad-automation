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

gdlParametersResponse = aclib.RunTapirCommand(
    "GetGDLParametersOfElements", {"elements": elements}
)


# Array (1D/2D) GDL parameters can be changed in two ways:
# - a list value resizes the array to match the given values
#     1D: 'value': [1, 2, 3]
#     2D: 'value': [[11, 12], [21, 22]]
# - 'index1' (and 'index2' for 2D arrays, both 1-based) changes a single item without resizing
#     'index1': 2, 'value': 5
# This example sets the first array parameter of each element to its current value.
elementsWithArrayParameters = []
for element, gdlParameters in zip(
    elements, gdlParametersResponse["gdlParametersOfElements"]
):
    arrayParameters = [
        p
        for p in gdlParameters.get("parameters", [])
        if "dimension1" in p and "value" in p and not p["isLocked"]
    ]
    if not arrayParameters:
        continue
    arrayParameter = arrayParameters[0]
    dim1: int = arrayParameter["dimension1"]
    dim2: int = arrayParameter["dimension2"]
    flatValues = arrayParameter["value"]
    if dim2 == 1:
        newValue = flatValues
    else:
        newValue = [flatValues[i * dim2 : (i + 1) * dim2] for i in range(dim1)]
    elementsWithArrayParameters.append(
        {
            "elementId": element["elementId"],
            "gdlParameters": [
                {
                    "name": arrayParameter["name"],
                    "index1": 1,
                    "index2": 1,
                    "value": flatValues[0],
                },
                {"name": arrayParameter["name"], "value": newValue},
            ],
        }
    )

if elementsWithArrayParameters:
    aclib.RunTapirCommand(
        "SetGDLParametersOfElements",
        {"elementsWithGDLParameters": elementsWithArrayParameters},
    )
