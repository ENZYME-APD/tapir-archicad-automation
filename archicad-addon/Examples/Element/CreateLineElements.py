"""Creates a 6 m long line with a filled arrowhead at its end with CreateLineElements.

The pen, line type, layer and arrowheads come from the Line tool defaults unless given.
An arrowhead given is shown unless its 'visible' is False; its size is in mm. Before
Archicad 29 both ends share one arrowhead type, size and pen.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateLineElements', {'linesData': [{
    'begCoordinate': {'x': 0.0, 'y': 0.0},
    'endCoordinate': {'x': 6.0, 'y': 0.0},
    'startArrow': {'visible': False},
    'endArrow': {'type': 'FullArrow30', 'size': 3.0}
}]}))
