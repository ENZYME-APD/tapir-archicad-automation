"""Changes the ID, name, clip polygon, pens and angle of a Detail marker with ModifyDetailMarkers, reads them back, then deletes it.

The script places its own Detail marker on the current floor plan story first (CreateDetailMarkers).
ModifyDetailMarkers works on Worksheet markers the same way. penIndex and lineTypeIndex are the
boundary's pen and line type, markerPenIndex the pen of the marker symbol; angle turns the marker
symbol unless isHorizontalMarker keeps it horizontal.
"""
import aclib

markers = aclib.RunTapirCommand ('CreateDetailMarkers', {
    'detailMarkersData': [{
        'clipPolygon': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 3.0}, {'x': 0.0, 'y': 3.0}],
        'name': 'Wall connection',
        'id': 'D1'
    }]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('ModifyDetailMarkers', {
    'detailMarkersWithDetails': [{
        'elementId': markers[0]['elementId'],
        'id': 'D2',
        'name': 'Corner detail',
        'clipPolygon': [{'x': 0.0, 'y': 1.0}, {'x': 4.0, 'y': 1.0}, {'x': 4.0, 'y': 4.0}, {'x': 0.0, 'y': 4.0}],
        'penIndex': 3,
        'lineTypeIndex': 1,
        'markerPenIndex': 5,
        'isHorizontalMarker': False,
        'angle': 0.5
    }]
}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': markers})['detailsOfElements'][0]['details']
print (details['detailIdStr'], details['detailName'], len (details['clipPolygon']), round (details['angle'], 3), details['isHorizontalMarker'])

aclib.RunTapirCommand ('DeleteElements', {'elements': markers})
