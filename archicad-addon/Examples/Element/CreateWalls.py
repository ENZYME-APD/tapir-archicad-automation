"""Creates a straight and a curved wall with CreateWalls.

Fields not given (structure, top link, ...) come from the Wall tool defaults, or from favoriteName.
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {
        'begCoordinate': {'x': 0.0, 'y': 0.0},
        'endCoordinate': {'x': 8.0, 'y': 0.0},
        'height': 3.0,
        'thickness': 0.3
    },
    {
        'begCoordinate': {'x': 0.0, 'y': 4.0},
        'endCoordinate': {'x': 8.0, 'y': 4.0},
        'arcAngle': math.pi / 4,
        'height': 3.0
    }
]}))
