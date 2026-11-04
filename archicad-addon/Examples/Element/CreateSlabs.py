"""Creates a 6 m x 5 m slab with a 1 m x 1 m hole with CreateSlabs.

level is the elevation of the slab's reference plane, in meters.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateSlabs', {'slabsData': [{
    'level': 0.0,
    'thickness': 0.25,
    'polygonCoordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 6.0, 'y': 0.0},
        {'x': 6.0, 'y': 5.0},
        {'x': 0.0, 'y': 5.0}
    ],
    'holes': [{
        'polygonOutline': [
            {'x': 2.0, 'y': 2.0},
            {'x': 3.0, 'y': 2.0},
            {'x': 3.0, 'y': 3.0},
            {'x': 2.0, 'y': 3.0}
        ]
    }]
}]}))
