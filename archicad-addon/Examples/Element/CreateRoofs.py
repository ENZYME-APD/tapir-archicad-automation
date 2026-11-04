"""Creates a multi-plane roof and a single-plane roof with CreateRoofs.

A pivotLine makes a single-plane roof that rises on the left of the line; angles are in radians.
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [
    {
        'level': 3.0,
        'polygonCoordinates': [
            {'x': 0.0, 'y': 0.0},
            {'x': 8.0, 'y': 0.0},
            {'x': 8.0, 'y': 6.0},
            {'x': 0.0, 'y': 6.0}
        ],
        'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (30)}]
    },
    {
        'level': 3.0,
        'polygonCoordinates': [
            {'x': 12.0, 'y': 0.0},
            {'x': 18.0, 'y': 0.0},
            {'x': 18.0, 'y': 4.0},
            {'x': 12.0, 'y': 4.0}
        ],
        'pivotLine': {
            'begCoordinate': {'x': 12.0, 'y': 0.0},
            'endCoordinate': {'x': 18.0, 'y': 0.0}
        },
        'angle': math.radians (15)
    }
]}))
