"""Creates a 6 m long, 3 m high curtain wall with 1.5 m wide columns and two rows with CreateCurtainWalls.

The frame and panel classes come from the Curtain Wall tool defaults, or from favoriteName.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateCurtainWalls', {'curtainWallsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 6.0, 'y': 0.0}],
    'zCoordinate': 0.0,
    'height': 3.0,
    'primaryPattern': {'sizes': [1.5], 'logic': 'FixedSizes'},
    'secondaryPattern': {'sizes': [1.0, 2.0], 'logic': 'FixedSizes'}
}]}))
