"""Creates a wall, then makes it longer, higher and thicker with ModifyWalls.

Only the given fields change. A new thickness only takes effect on a Basic wall.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 4.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyWalls', {'wallsWithDetails': [{
    'elementId': wall['elementId'],
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.5,
    'structureType': 'Basic',
    'thickness': 0.4
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [wall], 'fields': ['details']})
