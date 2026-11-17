"""Draws a circle with a 2 m radius and dimensions its radius with CreateRadialDimensions, then reads the dimension back.

The dimension is linked to the circle (elementId) and its dimension line ends at dimensionLineEnd; it
can also measure an arc or a curved wall. GetDetailsOfElements returns the radius Archicad measured,
the owner element and how the link is stored. The example deletes what it created.
"""
import aclib

circles = aclib.RunTapirCommand ('CreateCircles', {'circlesData': [{
    'origin': {'x': 0.0, 'y': 0.0},
    'radius': 2.0
}]})['elements']

result = aclib.RunTapirCommand ('CreateRadialDimensions', {'radialDimensionsData': [{
    'elementId': circles[0]['elementId'],
    'dimensionLineEnd': {'x': 2.5, 'y': 2.5},
    'textDirection': 'Radial',
    'showOrigin': True
}]})
aclib.PrintJson (result)

dimensions = [item for item in result['elements'] if 'elementId' in item]
if dimensions:
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': dimensions})
    aclib.PrintJson (details)
    print ('Measured radius: {:.3f} m'.format (details['detailsOfElements'][0]['details']['radius']))

aclib.RunTapirCommand ('DeleteElements', {'elements': dimensions + circles})
