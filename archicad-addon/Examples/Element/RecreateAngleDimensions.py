"""Dimensions a 30 degree roof pitch with CreateAngleDimensions, then moves the dimension onto a 45 degree pitch with RecreateAngleDimensions.

Archicad does not move the lines or the arc of an angle dimension through ACAPI_Element_Change, so
RecreateAngleDimensions creates it again on the new lines and arc position (as CreateAngleDimensions takes
them) with the old one's pens, text, markers and inner or outer arc, classifications and property
values, and deletes the old one. The result gives the new angle dimension. Archicad 29 gives angle
dimensions no Element ID, so there is none to carry.
"""
import math
import aclib

def Line (angle, length = 4.0):
    return {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': length * math.cos (angle), 'y': length * math.sin (angle)}}

def ArcPosition (angle, radius = 3.0):
    return {'x': radius * math.cos (angle / 2.0), 'y': radius * math.sin (angle / 2.0)}

pitch = math.radians (30.0)
dimension = aclib.RunTapirCommand ('CreateAngleDimensions', {'angleDimensionsData': [{
    'firstLine': Line (0.0),
    'secondLine': Line (pitch),
    'arcPosition': ArcPosition (pitch),
    'smallArc': True,
    'textPen': 3
}]})['elements'][0]

newPitch = math.radians (45.0)
result = aclib.RunTapirCommand ('RecreateAngleDimensions', {'angleDimensions': [{
    'elementId': dimension['elementId'],
    'firstLine': Line (0.0),
    'secondLine': Line (newPitch),
    'arcPosition': ArcPosition (newPitch, 2.0)
}]})
aclib.PrintJson (result)

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}]})
print ('Measured angle: {:.1f} degrees'.format (math.degrees (details['detailsOfElements'][0]['details']['measuredAngle'])))
