"""Clouds an area with CreateChangeMarkers, creating a new Change for it, and links a second cloud to the same Change.

The markers are placed in the active window (the floor plan story, a section, a detail, a worksheet or a layout).
The Change ID must not exist yet in the project.
"""
import aclib

changeId = 'TAPIR-EX-1'

result = aclib.RunTapirCommand ('CreateChangeMarkers', {
    'changeMarkersData': [
        {
            'polygon': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 3.0}, {'x': 0.0, 'y': 3.0}],
            'markerPosition': {'x': 5.0, 'y': 3.5},
            'changeId': changeId,
            'createChange': True,
            'changeDescription': 'Opening moved'
        },
        {
            'polygon': [{'x': 8.0, 'y': 0.0}, {'x': 10.0, 'y': 0.0}, {'x': 9.0, 'y': 2.0}],
            'markerComponent': 'CloudOnly',
            'changeId': changeId
        }
    ]
})
aclib.PrintJson (result)

aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': [element for element in result['elements'] if 'elementId' in element]
})
aclib.RunTapirCommand ('GetRevisionChanges')
