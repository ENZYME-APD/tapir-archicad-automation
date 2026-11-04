"""Creates a wall and places a 1 m x 2.1 m door in it with CreateDoors.

centerOffset is measured along the wall from its begin point. The library part comes from the Door
tool defaults, or from libraryPartName or favoriteName.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateDoors', {'doorsData': [{
    'ownerWallId': wall['elementId'],
    'centerOffset': 2.0,
    'width': 1.0,
    'height': 2.1
}]}))
