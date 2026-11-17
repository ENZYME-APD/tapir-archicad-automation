"""Creates two walls and a door in the first one, then moves the door into the second wall with RecreateDoors, keeping its position along the wall.

Archicad does not move a door into another wall through ModifyDoors (ACAPI_Element_Change keeps its
owner), so RecreateDoors creates it again in ownerWallId with the old door's settings, library part and
GDL parameters, and deletes the old one. Without centerOffset the door keeps its old position along the
wall. The result gives the new door and lists what could not be carried over (labels, dimensions, group).
"""
import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 6.0, 'y': 0.0}, 'height': 3.0},
    {'begCoordinate': {'x': 0.0, 'y': 5.0}, 'endCoordinate': {'x': 6.0, 'y': 5.0}, 'height': 3.0}
]})['elements']

door = aclib.RunTapirCommand ('CreateDoors', {'doorsData': [{
    'ownerWallId': walls[0]['elementId'],
    'centerOffset': 4.0,
    'width': 0.9,
    'height': 2.1,
    'id': 'DOOR-01'
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateDoors', {'doors': [{
    'elementId': door['elementId'],
    'ownerWallId': walls[1]['elementId']
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}], 'fields': ['details']}))
