"""Creates a polyline, then changes its pen, dash mode, end arrowhead and points with ModifyPolylines.

The settings have the keys CreatePolylines takes and GetDetailsOfElements returns; only the given
ones are changed. coordinates replaces all the points of the polyline (arcs only come with it);
the polyline keeps its GUID.
"""
import math
import aclib

polyline = aclib.RunTapirCommand ('CreatePolylines', {'polylinesData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 3.0, 'y': 3.0}, {'x': 6.0, 'y': 0.0}]
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyPolylines', {'polylinesWithDetails': [{
    'elementId': polyline['elementId'],
    'linePenIndex': 3,
    'drawSegmentMode': 'Continuous',
    'endArrow': {'type': 'FullArrow30', 'size': 3.0},
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 3.0, 'y': 3.0}, {'x': 6.0, 'y': 0.0}, {'x': 9.0, 'y': 0.0}],
    'arcs': [{'begIndex': 2, 'endIndex': 3, 'arcAngle': math.pi / 2}]
}]}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [polyline]}))
