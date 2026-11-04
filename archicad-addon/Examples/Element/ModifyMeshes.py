"""Creates a flat mesh, then gives it a skirt and a raised corner with ModifyMeshes.

Only the fields given in meshData change; polygonCoordinates replaces the whole outline, z being
the height of each point above level.
"""
import aclib

mesh = aclib.RunTapirCommand ('CreateMeshes', {'meshesData': [{
    'level': 0.0,
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 0.0, 'z': 0.0},
        {'x': 10.0, 'y': 10.0, 'z': 0.0},
        {'x': 0.0, 'y': 10.0, 'z': 0.0}
    ]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyMeshes', {'meshesData': [{
    'elementId': mesh['elementId'],
    'meshData': {
        'skirtType': 'SolidBodyWithSkirt',
        'skirtLevel': 1.0,
        'ridges': 'AllSmooth',
        'polygonCoordinates': [
            {'x': 0.0, 'y': 0.0, 'z': 0.0},
            {'x': 10.0, 'y': 0.0, 'z': 0.0},
            {'x': 10.0, 'y': 10.0, 'z': 3.0},
            {'x': 0.0, 'y': 10.0, 'z': 0.0}
        ]
    }
}]}))
