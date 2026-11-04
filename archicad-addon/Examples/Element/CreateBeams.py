"""Creates a 6 m long beam at 3 m height with CreateBeams.

With the TopCenter anchor the beam hangs below its reference line at zCoordinate.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateBeams', {'beamsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'zCoordinate': 3.0,
    'width': 0.3,
    'height': 0.5,
    'isWidthAndHeightLinked': False,
    'anchorPoint': 'TopCenter'
}]}))
