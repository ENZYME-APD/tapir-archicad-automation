"""Creates a 10 m x 10 m mesh that rises from 0 to 2 m with CreateMeshes.

z of each outline point is its height above level.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMeshes', {'meshesData': [{
    'level': 0.0,
    'skirtType': 'SolidBodyWithSkirt',
    'skirtLevel': 1.0,
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 10.0, 'z': 2.0},
        {'x': 0.0, 'y': 10.0, 'z': 2.0}
    ]
}]}))
