"""Regularizes a self-intersecting (bow tie) outline with a repeated point with RegularizePolygons.

The results are in the format of CreateSlabs; nothing is created.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('RegularizePolygons', {
    'polygons': [{
        'polygonCoordinates': [
            {'x': 0.0, 'y': 0.0},
            {'x': 4.0, 'y': 4.0},
            {'x': 4.0, 'y': 4.0},
            {'x': 4.0, 'y': 0.0},
            {'x': 0.0, 'y': 4.0}
        ]
    }],
    'withAncestry': False
}))
