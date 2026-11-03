import aclib

# A Detail marker and a Worksheet marker on the current floor plan story, each cutting out a
# 4 x 3 m rectangle. Archicad creates a new Detail and a new Worksheet viewpoint for them.
def Rectangle (x, y):
    return [
        {'x': x, 'y': y},
        {'x': x + 4.0, 'y': y},
        {'x': x + 4.0, 'y': y + 3.0},
        {'x': x, 'y': y + 3.0}
    ]

details = aclib.RunTapirCommand ('CreateDetailMarkers', {
    'detailMarkersData': [{
        'clipPolygon': Rectangle (0.0, 0.0),
        'name': 'Wall connection',
        'id': 'D1'
    }]
})['elements']

worksheets = aclib.RunTapirCommand ('CreateWorksheetMarkers', {
    'worksheetMarkersData': [{
        'clipPolygon': Rectangle (6.0, 0.0),
        'name': 'Facade sketch',
        'id': 'W1'
    }]
})['elements']

markers = details + worksheets

# ModifyDetailMarkers changes the name, the ID and the clip polygon of existing markers.
aclib.RunTapirCommand ('ModifyDetailMarkers', {
    'detailMarkersWithDetails': [
        {'elementId': details[0]['elementId'], 'id': 'D2', 'clipPolygon': Rectangle (0.0, 1.0)},
        {'elementId': worksheets[0]['elementId'], 'name': 'Facade study'}
    ]
})

for marker in aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': markers})['detailsOfElements']:
    details = marker['details']
    print (marker['type'], details['detailIdStr'], details['detailName'], len (details['clipPolygon']))

aclib.RunTapirCommand ('DeleteElements', {
    'elements': markers
})
