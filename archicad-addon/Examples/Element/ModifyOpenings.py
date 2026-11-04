"""Creates a wall with an opening, then resizes and moves the opening with ModifyOpenings.

Requires Archicad 28 or earlier; from Archicad 29 use SetOpeningDetails instead.
"""
import aclib

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 8.0, 'y': 0.0},
    'height': 3.0
}]})['elements'][0]

opening = aclib.RunTapirCommand ('CreateOpenings', {'openingsData': [{
    'ownerElementId': wall['elementId'],
    'basePoint': {'x': 2.0, 'y': 0.0, 'z': 1.0},
    'width': 0.8,
    'height': 0.8
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyOpenings', {'openingsWithDetails': [{
    'elementId': opening['elementId'],
    'basePoint': {'x': 5.0, 'y': 0.0, 'z': 0.5},
    'width': 1.2,
    'height': 1.5
}]}))
