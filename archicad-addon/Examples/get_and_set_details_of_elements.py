import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['IsVisibleIn3D']})['elements']
columns = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Column', 'filters': ['IsVisibleIn3D']})['elements']

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls + columns})['detailsOfElements']

wallsWithChangedDetails = []
for i in range(len(walls)):
    wallsWithChangedDetails.append ({
        'elementId': walls[i]['elementId'],
        'details': {
            'layerIndex': detailsOfElements[i]['layerIndex'] + 1,
            'floorIndex': detailsOfElements[i]['floorIndex'] + 1
        }
    })

response = aclib.RunTapirCommand ('SetDetailsOfElements', {'elementsWithDetails': wallsWithChangedDetails})

detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls})['detailsOfElements']