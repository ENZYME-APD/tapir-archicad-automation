"""Creates a 6 m long beam at 3 m height with its own cut fill pen and two holes with CreateBeams.

With the TopCenter anchor the beam hangs below its reference line at zCoordinate. CreateBeams takes the same
cut fill pens (written from Archicad 27), cover fill and holes as ModifyBeams; centerX of a hole is measured
along the beam from begCoordinate.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateBeams', {'beamsData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'zCoordinate': 3.0,
    'width': 0.3,
    'height': 0.5,
    'isWidthAndHeightLinked': False,
    'anchorPoint': 'TopCenter',
    'cutFillPen': {'overridden': True, 'penIndex': 5},
    'holes': [
        {'type': 'Rectangular', 'centerX': 2.0, 'centerZ': 0.0, 'width': 0.18, 'height': 0.12, 'showContour': True},
        {'type': 'Circular', 'centerX': 4.0, 'centerZ': 0.0, 'width': 0.15, 'showContour': True}
    ]
}]}))
