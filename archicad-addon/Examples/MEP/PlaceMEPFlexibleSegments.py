"""Places a flexible duct through three control points with PlaceMEPFlexibleSegments.

Uses the current settings of the Flexible Segment tool. Coordinates are in meters. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('PlaceMEPFlexibleSegments', {
    'flexibleSegmentsData': [{
        'domain': 'Ventilation',
        'controlPoints': [
            {'x': 0.0, 'y': 0.0, 'z': 2.5},
            {'x': 1.5, 'y': -0.5, 'z': 2.5},
            {'x': 3.0, 'y': 0.0, 'z': 2.5}
        ]
    }]
}))
