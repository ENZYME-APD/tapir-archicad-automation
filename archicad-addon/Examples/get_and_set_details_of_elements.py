import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})['detailsOfElements']

elementsWithChangedDetails = []
for i in range(len(elements)):
    elementsWithChangedDetails.append ({
        'elementId': elements[i]['elementId'],
        'details': {
            'layerIndex': detailsOfElements[i]['layerIndex'] + 1,
            'floorIndex': detailsOfElements[i]['floorIndex'] + 1
        }
    })

response = aclib.RunTapirCommand ('SetDetailsOfElements', {'elementsWithDetails': elementsWithChangedDetails})

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})['detailsOfElements']