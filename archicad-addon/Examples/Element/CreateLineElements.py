"""Creates a 6 m long line with CreateLineElements.

The pen, line type and layer come from the Line tool defaults unless given.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateLineElements', {'linesData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0}
}]}))
