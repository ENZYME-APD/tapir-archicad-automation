"""Trims two walls to the underside of a roof with TrimElements and reads the trims back.

The walls and the roof are created by the script and deleted at the end.
"""
import math

import aclib

# Two walls under a pitched roof, created by this script and deleted at the end.
walls = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {'begCoordinate': {'x': 100.0, 'y': 100.0}, 'endCoordinate': {'x': 106.0, 'y': 100.0}, 'height': 4.0, 'floorIndex': 0},
    {'begCoordinate': {'x': 106.0, 'y': 100.0}, 'endCoordinate': {'x': 106.0, 'y': 105.0}, 'height': 4.0, 'floorIndex': 0}
]})['elements']
roofs = aclib.RunTapirCommand ('CreateRoofs', {'roofsData': [{
    'level': 2.7,
    'floorIndex': 0,
    'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (25.0)}],
    'polygonCoordinates': [{'x': 100.0, 'y': 100.0}, {'x': 106.0, 'y': 100.0}, {'x': 106.0, 'y': 105.0}, {'x': 100.0, 'y': 105.0}]
}]})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('TrimElements', {
    'elements': walls,
    'trimmingElement': roofs[0]['elementId'],
    'trimType': 'KeepInside'
}))

aclib.RunTapirCommand ('GetElementTrims', {'elements': walls})

aclib.RunTapirCommand ('DeleteElements', {'elements': walls + roofs})
