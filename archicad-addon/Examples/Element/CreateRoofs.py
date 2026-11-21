"""Creates a multi-plane roof, a single-plane roof and a gable roof with CreateRoofs.

A pivotLine makes a single-plane roof that rises on the left of the line; angles are in radians.
pivotEdges sets the planes of the edges of a multi-plane roof (one item per edge, in the order of
polygonCoordinates): angleType Gable makes a gable on that edge. id and layerIndex set the
Element ID and the layer of the new roof.
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
    },
    {
        'id': 'ROOF-GABLE',
        'layerIndex': 1,
        'level': 3.0,
        'polygonCoordinates': [
            {'x': 22.0, 'y': 0.0},
            {'x': 30.0, 'y': 0.0},
            {'x': 30.0, 'y': 6.0},
            {'x': 22.0, 'y': 6.0}
        ],
        'levels': [{'levelHeight': 10.0, 'levelAngle': math.radians (30)}],
        'pivotEdges': [
            {},
            {'levels': [{'angleType': 'Gable'}]},
            {},
            {'levels': [{'angleType': 'Gable'}]}
        ]
    }
]}))
