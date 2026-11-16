"""Creates a 30 degree angle dimension, then turns its second line to 45 degrees and changes its pen and text with ModifyAngleDimensions.

The settings have the keys CreateAngleDimensions takes and GetDetailsOfElements returns; only the
given ones are changed, and a line or arc position not given keeps the current one. A dimension
given new lines or a new arc position becomes static.
"""
import math
import aclib

def Line (angle):
    return {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0 * math.cos (angle), 'y': 4.0 * math.sin (angle)}}

def ArcPosition (angle):
    return {'x': 3.0 * math.cos (angle / 2.0), 'y': 3.0 * math.sin (angle / 2.0)}

dimension = aclib.RunTapirCommand ('CreateAngleDimensions', {'angleDimensionsData': [{
    'firstLine': Line (0.0),
    'secondLine': Line (math.radians (30.0)),
    'arcPosition': ArcPosition (math.radians (30.0)),
    'smallArc': True
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyAngleDimensions', {'angleDimensionsWithDetails': [{
    'elementId': dimension['elementId'],
    'secondLine': Line (math.radians (45.0)),
    'arcPosition': ArcPosition (math.radians (45.0)),
    'pen': 3,
    'textHeight': 3.0,
    'textPen': 5
}]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [dimension]})
print ('Measured angle: {:.1f} degrees'.format (math.degrees (details['detailsOfElements'][0]['details']['measuredAngle'])))
