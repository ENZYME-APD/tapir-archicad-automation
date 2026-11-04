"""Subtracts a slab from a wall with CreateSolidElementLinks and reads the link back.

The wall and the slab are created by the script and deleted at the end.
"""
import aclib

# A wall and a slab cutting through it, created by this script and deleted at the end.
wall = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 100.0, 'y': 100.0}, 'endCoordinate': {'x': 106.0, 'y': 100.0}, 'height': 3.0, 'floorIndex': 0}]
})['elements'][0]
slab = aclib.RunTapirCommand ('CreateSlabs', {
    'slabsData': [{
        'level': 1.5,
        'thickness': 0.3,
        'polygonCoordinates': [{'x': 102.0, 'y': 99.0}, {'x': 104.0, 'y': 99.0}, {'x': 104.0, 'y': 101.0}, {'x': 102.0, 'y': 101.0}]
    }]
})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateSolidElementLinks', {
    'solidLinks': [{
        'targetId': wall['elementId'],
        'operatorId': slab['elementId'],
        'operation': 'Subtraction',
        'linkFlags': {'inheritOperatorAttributes': False, 'skipPolygonHoles': False}
    }]
}))

aclib.RunTapirCommand ('GetSolidElementLinks', {'elements': [wall]})

aclib.RunTapirCommand ('DeleteElements', {'elements': [wall, slab]})
