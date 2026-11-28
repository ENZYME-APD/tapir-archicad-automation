"""Dimensions the radius of a circle, then changes its pens, prefix, text and text box with ModifyRadialDimensions.

The settings have the keys CreateRadialDimensions takes and GetDetailsOfElements returns; only the
given ones are changed (also inside marker and textBox). The measured element and the dimension line
are not changed: delete the dimension and create it again with CreateRadialDimensions for a new
geometry. The example deletes what it created.
"""
import aclib

circles = aclib.RunTapirCommand ('CreateCircles', {'circlesData': [{
    'origin': {'x': 0.0, 'y': 0.0},
    'radius': 2.0
}]})['elements']

dimensions = [item for item in aclib.RunTapirCommand ('CreateRadialDimensions', {'radialDimensionsData': [{
    'elementId': circles[0]['elementId'],
    'dimensionLineEnd': {'x': 2.5, 'y': 2.5}
}]})['elements'] if 'elementId' in item]

if dimensions:
    aclib.PrintJson (aclib.RunTapirCommand ('ModifyRadialDimensions', {'radialDimensionsWithDetails': [{
        'elementId': dimensions[0]['elementId'],
        'linePen': 3,
        'textPen': 5,
        'prefix': 'R=',
        'showOrigin': True,
        'marker': {'type': 'FullArrow30'},
        'textBox': {'frame': True, 'framePen': 2, 'opaque': True, 'backgroundPen': 1}
    }]}))

    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': dimensions})
    aclib.PrintJson (details)

aclib.RunTapirCommand ('DeleteElements', {'elements': dimensions + circles})
