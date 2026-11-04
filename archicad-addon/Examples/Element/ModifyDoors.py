"""Creates a wall with a door, then mirrors the door and moves it along the wall with ModifyDoors.

Only the given fields change. centerOffset is measured along the wall from its begin point.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

door = aclib.RunTapirCommand ('CreateDoors', {'doorsData': [{
    'ownerWallId': wall['elementId'],
    'centerOffset': 2.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyDoors', {'doorsWithDetails': [{
    'elementId': door['elementId'],
    'centerOffset': 4.0,
    'width': 0.9,
    'reflected': True
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [door], 'fields': ['details']})
