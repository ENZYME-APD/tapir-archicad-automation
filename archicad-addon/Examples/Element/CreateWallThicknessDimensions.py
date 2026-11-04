"""Creates a wall and dimensions its thickness with CreateWallThicknessDimensions.

The dimension line runs across the wall (direction) through referencePoint.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0,
    'thickness': 0.3
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateWallThicknessDimensions', {'dimensionsData': [{
    'wallId': wall['elementId'],
    'referencePoint': {'x': 3.0, 'y': -1.0},
    'direction': {'x': 0.0, 'y': 1.0}
}]}))
