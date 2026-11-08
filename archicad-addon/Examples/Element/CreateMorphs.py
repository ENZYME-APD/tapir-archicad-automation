"""Creates a 2 m box morph and a tetrahedron morph from vertices and faces with CreateMorphs.

A body's vertices are local to the morph's basePoint; each face lists vertex indices counterclockwise
as seen from outside. An item may also name a Morph favorite to start from (favoriteName), a layer
(layerIndex) and an Element ID (id), as the box does with its id.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMorphs', {'morphsData': [
    {
        'id': 'MORPH-BOX',
        'basePoint': {'x': 0.0, 'y': 0.0, 'z': 0.0},
        'size': {'x': 2.0, 'y': 2.0, 'z': 2.0}
    },
    {
        'basePoint': {'x': 4.0, 'y': 0.0, 'z': 0.0},
        'body': {
            'vertices': [
                {'x': 0.0, 'y': 0.0, 'z': 0.0},
                {'x': 2.0, 'y': 0.0, 'z': 0.0},
                {'x': 0.0, 'y': 2.0, 'z': 0.0},
                {'x': 0.0, 'y': 0.0, 'z': 2.0}
            ],
            'polygons': [
                {'vertexIds': [0, 2, 1]},
                {'vertexIds': [0, 1, 3]},
                {'vertexIds': [1, 2, 3]},
                {'vertexIds': [0, 3, 2]}
            ]
        }
    }
]}))
