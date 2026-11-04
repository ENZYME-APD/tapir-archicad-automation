"""Creates an open and a closed spline through the given points with CreateSplines.

Archicad smooths the curve through the points itself.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateSplines', {'splinesData': [
    {
        'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 2.0, 'y': 3.0}, {'x': 4.0, 'y': 0.0}]
    },
    {
        'coordinates': [{'x': 6.0, 'y': 0.0}, {'x': 8.0, 'y': 2.0}, {'x': 10.0, 'y': 0.0}, {'x': 8.0, 'y': -2.0}],
        'closed': True
    }
]}))
