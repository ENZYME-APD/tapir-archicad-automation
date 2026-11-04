"""Creates a wall and places a 1 m x 1.5 m window in it with CreateWindows.

centerOffset is measured along the wall from its begin point. The library part comes from the Window
tool defaults, or from libraryPartName or favoriteName.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateWindows', {'windowsData': [{
    'ownerWallId': wall['elementId'],
    'centerOffset': 3.0,
    'sillHeight': 0.9,
    'width': 1.0,
    'height': 1.5
}]}))
