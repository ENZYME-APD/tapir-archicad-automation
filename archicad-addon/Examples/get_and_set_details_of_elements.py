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
    elif detailsOfElements[i]['type'] == 'Zone' and not detailsOfElements[i]['details']['isManual']:
        # An automatic zone also reports the point it was placed from (referencePosition);
        # moving it changes where the zone is found when zones are next updated.
        origReferencePosition = detailsOfElements[i]['details']['referencePosition']
        elementWithChangedDetails['details'].update({
            'typeSpecificDetails': {
                'referencePosition': {
                    'x': origReferencePosition['x'] + 0.1,
                    'y': origReferencePosition['y'] + 0.1
                }
            }
        })

    elementsWithChangedDetails.append (elementWithChangedDetails)

response = aclib.RunTapirCommand ('SetDetailsOfElements', {'elementsWithDetails': elementsWithChangedDetails})

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': elements})['detailsOfElements']