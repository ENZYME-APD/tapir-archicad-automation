"""Places a Worksheet marker on the current floor plan story with CreateWorksheetMarkers, then deletes it again.

The marker cuts out a 4 x 3 m rectangle; Archicad creates a new Worksheet viewpoint for it, which is deleted
together with the marker.
"""
import aclib

worksheets = aclib.RunTapirCommand ('CreateWorksheetMarkers', {
    'worksheetMarkersData': [{
        'clipPolygon': [{'x': 6.0, 'y': 0.0}, {'x': 10.0, 'y': 0.0}, {'x': 10.0, 'y': 3.0}, {'x': 6.0, 'y': 3.0}],
        'name': 'Facade sketch',
        'id': 'W1'
    }]
})['elements']
aclib.PrintJson (worksheets)

aclib.RunTapirCommand ('DeleteElements', {'elements': worksheets})
