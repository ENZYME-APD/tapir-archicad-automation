import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})['detailsOfElements']

elementsWithChangedDetails = []
for i in range(len(elements)):
    elementWithChangedDetails = {
        'elementId': elements[i]['elementId'],
        'details': {
            'layerIndex': detailsOfElements[i]['layerIndex'] + 1,
            'floorIndex': detailsOfElements[i]['floorIndex'] + 1
        }
    }
    if detailsOfElements[i]['type'] == 'Wall':
        origBegCoordinate = detailsOfElements[i]['details']['begCoordinate']
        origEndCoordinate = detailsOfElements[i]['details']['endCoordinate']
        elementWithChangedDetails['details'].update({
            'typeSpecificDetails': {
                'begCoordinate': {
                    'x': origBegCoordinate['x'] + 0.4,
                    'y': origBegCoordinate['y'] + 0.6
                },
                'endCoordinate': {
                    'x': origEndCoordinate['x'] + 0.3,
                    'y': origEndCoordinate['y'] + 0.7
                }
            }
        })

    elementsWithChangedDetails.append (elementWithChangedDetails)

response = aclib.RunTapirCommand ('SetDetailsOfElements', {'elementsWithDetails': elementsWithChangedDetails})

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})['detailsOfElements']