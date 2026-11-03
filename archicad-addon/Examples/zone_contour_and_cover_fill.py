import aclib

# The contour line and the cover fill of a zone on the floor plan. CreateZones takes them as
# optional fields, GetDetailsOfElements returns them, and SetDetailsOfElements changes only
# the given fields, so the category and the zone stamp stay as they are.
lineTypes = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes']
fills = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Fill'})['attributes']

elements = aclib.RunTapirCommand ('CreateZones', {
    'zonesData': [
        {
            'name': 'Storage',
            'numberStr': '102',
            'floorIndex': 0,
            'contourLine': {
                'use': True,
                'pen': 3,
                'lineTypeId': lineTypes[0]['attributeId']
            },
            'coverFill': {
                'use': True,
                'fillId': fills[0]['attributeId'],
                'foregroundPen': 5,
                'backgroundPen': 0
            },
            'geometry': {
                'polygonCoordinates': [
                    { 'x': 100.0, 'y': 0.0 },
                    { 'x': 104.0, 'y': 0.0 },
                    { 'x': 104.0, 'y': 3.0 },
                    { 'x': 100.0, 'y': 3.0 }
                ]
            }
        }
    ]
})['elements']

zoneId = elements[0]['elementId']

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': zoneId}]})

# Only the contour pen and the line type change; the cover fill keeps its settings.
aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': zoneId,
            'details': {
                'typeSpecificDetails': {
                    'contourLine': {
                        'pen': 7,
                        'lineTypeId': lineTypes[-1]['attributeId']
                    }
                }
            }
        }
    ]
})

aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': zoneId}]})

aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': zoneId}]})
