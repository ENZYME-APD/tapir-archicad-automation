"""Creates two walls and a window in the first one, then moves the window into the second wall with RecreateWindows.

Archicad does not move a window into another wall through ModifyWindows (ACAPI_Element_Change keeps
its owner), so RecreateWindows creates it again in ownerWallId, at centerOffset along it (or at its old
centerOffset), with the old window's settings, library part and GDL parameters, and deletes the old one.
The result gives the new window and lists what could not be carried over (labels, dimensions, group);
the new window keeps the Element ID, classifications and property values.
"""
import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 6.0, 'y': 0.0}, 'height': 3.0},
    {'begCoordinate': {'x': 0.0, 'y': 5.0}, 'endCoordinate': {'x': 6.0, 'y': 5.0}, 'height': 3.0}
]})['elements']

window = aclib.RunTapirCommand ('CreateWindows', {'windowsData': [{
    'ownerWallId': walls[0]['elementId'],
    'centerOffset': 2.0,
    'width': 1.2,
    'height': 1.4,
    'sillHeight': 0.9,
    'id': 'WIN-01'
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateWindows', {'windows': [{
    'elementId': window['elementId'],
    'ownerWallId': walls[1]['elementId'],
    'centerOffset': 3.0
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}], 'fields': ['details']}))
