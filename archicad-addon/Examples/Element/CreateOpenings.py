"""Creates a wall and cuts a 0.8 m x 0.8 m opening into it with CreateOpenings.

basePoint is the bottom left corner of the opening in world coordinates.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateOpenings', {'openingsData': [{
    'ownerElementId': wall['elementId'],
    'basePoint': {'x': 2.0, 'y': 0.0, 'z': 1.0},
    'width': 0.8,
    'height': 0.8
}]}))
