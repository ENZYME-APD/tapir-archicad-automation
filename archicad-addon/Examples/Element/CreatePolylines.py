"""Creates a polyline of two straight segments and one arc segment with CreatePolylines.

arcAngle is in radians; arcs refer to the indices of the coordinates.
drawSegmentMode Continuous runs the line type's dashes over the whole polyline
instead of restarting them on every segment (SegmentBySegment).
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreatePolylines', {'polylinesData': [{
    'coordinates': [
        {'x': 0.0, 'y': 0.0},
        {'x': 3.0, 'y': 3.0},
        {'x': 6.0, 'y': 0.0},
        {'x': 9.0, 'y': 0.0}
    ],
    'arcs': [{'begIndex': 2, 'endIndex': 3, 'arcAngle': math.pi / 2}],
    'drawSegmentMode': 'Continuous'
}]}))
