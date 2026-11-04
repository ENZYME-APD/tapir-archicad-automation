"""Creates a railing that climbs 1 m along its second segment and ends in a quarter arc with CreateRailingsFromPolyline.

z of each node is its elevation above the home story. The posts, rails and infill come from the
Railing tool defaults, or from favoriteName.
"""
import aclib
import math

aclib.PrintJson (aclib.RunTapirCommand ('CreateRailingsFromPolyline', {'railingsData': [{
    'polylineCoordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 4.0, 'y': 0.0, 'z': 0.0},
        {'x': 8.0, 'y': 0.0, 'z': 1.0},
        {'x': 10.0, 'y': 2.0, 'z': 1.0}
    ],
    'polylineArcs': [{'begIndex': 2, 'endIndex': 3, 'arcAngle': math.pi / 2}],
    'floorIndex': 0,
    'height': 1.1
}]}))
