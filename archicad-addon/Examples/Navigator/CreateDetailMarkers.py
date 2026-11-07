"""Places a Detail marker on the current floor plan story with CreateDetailMarkers, then deletes it again.

The marker cuts out a 4 x 3 m rectangle; Archicad creates a new Detail viewpoint for it, which is deleted
together with the marker.
"""
import aclib

details = aclib.RunTapirCommand ('CreateDetailMarkers', {
    'detailMarkersData': [{
        'clipPolygon': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 3.0}, {'x': 0.0, 'y': 3.0}],
        'name': 'Wall connection',
        'id': 'D1'
    }]
})['elements']
aclib.PrintJson (details)

aclib.RunTapirCommand ('DeleteElements', {'elements': details})
