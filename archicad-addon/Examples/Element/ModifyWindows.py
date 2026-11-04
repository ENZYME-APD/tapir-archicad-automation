"""Creates a wall with a window, then makes the window wider and moves it along the wall with ModifyWindows.

Only the given fields change. centerOffset is measured along the wall from its begin point.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

window = aclib.RunTapirCommand ('CreateWindows', {'windowsData': [{
    'ownerWallId': wall['elementId'],
    'centerOffset': 2.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyWindows', {'windowsWithDetails': [{
    'elementId': window['elementId'],
    'centerOffset': 3.5,
    'width': 1.5,
    'sillHeight': 1.0
}]}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [window], 'fields': ['details']})
