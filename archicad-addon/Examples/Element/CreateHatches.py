"""Creates a 4 m x 4 m hatch with a 1 m x 1 m hole and a red foreground with CreateHatches.

Don't repeat the first point at the end of the outline. Colors are RGB values between 0 and 1.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateHatches', {'hatchesData': [{
    'coordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 4.0, 'y': 0.0},
        {'x': 4.0, 'y': 4.0},
        {'x': 0.0, 'y': 4.0}
    ],
    'holes': [{
        'polygonOutline': [
            {'x': 1.0, 'y': 1.0},
            {'x': 2.0, 'y': 1.0},
            {'x': 2.0, 'y': 2.0},
            {'x': 1.0, 'y': 2.0}
        ]
    }],
    'foregroundColor': {'red': 1.0, 'green': 0.0, 'blue': 0.0}
}]}))
