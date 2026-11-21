"""Clouds an area with CreateChangeMarkers, then links the marker to a second, new Change and moves and turns its head with ModifyChangeMarkers.

The settings have the keys CreateChangeMarkers takes and GetDetailsOfElements returns; only the
given ones are changed. The cloud polygon cannot be changed. The Change IDs must not exist yet.
"""
import aclib

marker = aclib.RunTapirCommand ('CreateChangeMarkers', {'changeMarkersData': [{
    'polygon': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 3.0}, {'x': 0.0, 'y': 3.0}],
    'markerPosition': {'x': 5.0, 'y': 3.5},
    'changeId': 'TAPIR-EX-MODIFY-1',
    'createChange': True,
    'changeDescription': 'Window moved'
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyChangeMarkers', {'changeMarkersWithDetails': [{
    'elementId': marker['elementId'],
    'changeId': 'TAPIR-EX-MODIFY-2',
    'createChange': True,
    'changeDescription': 'Window moved again',
    'markerPosition': {'x': 6.0, 'y': 4.0},
    'angle': 0.5
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [marker]}))
