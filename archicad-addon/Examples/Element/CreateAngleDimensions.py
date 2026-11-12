"""Draws a roof pitch line and dimensions its 30 degree angle to the horizontal with CreateAngleDimensions, then reads the measured angle back.

Each line is given by two points; the dimension arc goes through arcPosition, whose side of the lines
chooses the measured angle. The dimension is created in the current window (the floor plan, or a
section or elevation), or in the database given by databaseId; favoriteName applies an Angle Dimension favorite.
"""
import math
import aclib

pitch = math.radians (30.0)
aclib.RunTapirCommand ('CreateLineElements', {'linesData': [
    {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0, 'y': 0.0}},
    {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0 * math.cos (pitch), 'y': 4.0 * math.sin (pitch)}}
]})

result = aclib.RunTapirCommand ('CreateAngleDimensions', {'angleDimensionsData': [{
    'firstLine': {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0, 'y': 0.0}},
    'secondLine': {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 4.0 * math.cos (pitch), 'y': 4.0 * math.sin (pitch)}},
    'arcPosition': {'x': 3.0 * math.cos (pitch / 2.0), 'y': 3.0 * math.sin (pitch / 2.0)},
    'smallArc': True
}]})
aclib.PrintJson (result)

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [result['elements'][0]]})
print ('Measured angle: {:.1f} degrees'.format (math.degrees (details['detailsOfElements'][0]['details']['measuredAngle'])))
