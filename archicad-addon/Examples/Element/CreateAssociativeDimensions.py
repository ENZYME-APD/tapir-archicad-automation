"""Creates a wall and dimensions its length with CreateAssociativeDimensions.

The dimension follows the wall: inIndex 1 and 2 are the end points of the wall's reference line. A
witness point given only by a coordinate is static. direction is the direction of the dimension line.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateAssociativeDimensions', {'dimensionsData': [{
    'referencePoint': {'x': 0.0, 'y': -1.0},
    'direction': {'x': 1.0, 'y': 0.0},
    'witnessPoints': [
        {'elementId': wall['elementId'], 'inIndex': 1},
        {'elementId': wall['elementId'], 'inIndex': 2},
        {'coordinate': {'x': 8.0, 'y': 0.0}}
    ]
}]}))
