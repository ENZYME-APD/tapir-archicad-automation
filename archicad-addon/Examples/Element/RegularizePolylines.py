"""Removes the repeated points of a polyline with RegularizePolylines, and reports which input point each result point came from.

The result is in the format of CreatePolylines; nothing is created.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('RegularizePolylines', {
    'polylines': [{
        'coordinates': [
            {'x': 0.0, 'y': 0.0},
            {'x': 2.0, 'y': 0.0},
            {'x': 2.0, 'y': 0.0},
            {'x': 2.0, 'y': 3.0}
        ]
    }],
    'withAncestry': True
}))
