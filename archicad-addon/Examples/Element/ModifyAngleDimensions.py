"""Creates a 30 degree angle dimension, then changes its pen, text, marker, witness lines, text box and measured arc with ModifyAngleDimensions.

The settings have the keys CreateAngleDimensions takes and GetDetailsOfElements returns; only the
given ones are changed (also inside marker and textBox). The lines and the arc position cannot be changed (Archicad keeps them):
delete the dimension and create it again with CreateAngleDimensions for a new geometry.
"""
import math
import aclib

def Line (angle):
    return {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0 * math.cos (angle), 'y': 4.0 * math.sin (angle)}}

dimension = aclib.RunTapirCommand ('CreateAngleDimensions', {'angleDimensionsData': [{
    'firstLine': Line (0.0),
    'secondLine': Line (math.radians (30.0)),
    'arcPosition': {'x': 3.0 * math.cos (math.radians (15.0)), 'y': 3.0 * math.sin (math.radians (15.0))},
    'smallArc': True
}]})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyAngleDimensions', {'angleDimensionsWithDetails': [{
    'elementId': dimension['elementId'],
    'smallArc': False,
    'pen': 3,
    'textHeight': 3.0,
    'textPen': 5,
    'textPosition': 'Below',
    'witnessForm': 'Fix',
    'marker': {'type': 'FullArrow30', 'size': 2.0},
    'textBox': {'frame': True, 'framePen': 2, 'opaque': True, 'backgroundPen': 1}
}]}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [dimension]})
print ('Measured angle: {:.1f} degrees'.format (math.degrees (details['detailsOfElements'][0]['details']['measuredAngle'])))
