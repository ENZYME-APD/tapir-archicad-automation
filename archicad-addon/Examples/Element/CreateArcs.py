"""Creates a half elliptic arc with a 2 m radius and an a/b ratio of 2 with CreateArcs.

Angles are in radians, counterclockwise from the X axis. Leave out ratio (or give
1.0) for a circular arc; angle turns the ellipse's 'a' axis.
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreateArcs', {'arcsData': [{
    'origin': {'x': 0.0, 'y': 0.0},
    'radius': 2.0,
    'begAngle': 0.0,
    'endAngle': math.pi,
    'ratio': 2.0
}]}))
