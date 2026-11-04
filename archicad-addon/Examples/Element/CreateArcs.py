"""Creates a half circle arc with a 2 m radius with CreateArcs.

Angles are in radians, counterclockwise from the X axis.
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreateArcs', {'arcsData': [{
    'origin': {'x': 0.0, 'y': 0.0},
    'radius': 2.0,
    'begAngle': 0.0,
    'endAngle': math.pi
}]}))
