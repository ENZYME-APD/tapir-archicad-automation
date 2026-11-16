"""Creates a 6 m long, 3 m high curtain wall with 1.5 m wide columns and two rows with CreateCurtainWalls, and a second one slanted at 75 degrees.

The frame and panel classes come from the Curtain Wall tool defaults, or from favoriteName.
angle is in degrees from the horizontal (90 is vertical, less leans to the left of the
reference line); a slanted curtain wall needs a straight reference line, and height stays
its vertical height.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5], 'logic': 'FixedSizes'},
    'secondaryPattern': {'sizes': [1.0, 2.0], 'logic': 'FixedSizes'}
}, {
    'coordinates': [{'x': 0.0, 'y': 5.0}, {'x': 6.0, 'y': 5.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'angle': 75.0,
    'primaryPattern': {'sizes': [1.5], 'logic': 'FixedSizes'},
    'secondaryPattern': {'sizes': [1.0, 2.0], 'logic': 'FixedSizes'}
}]}))
