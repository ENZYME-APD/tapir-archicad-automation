"""Asks the user to draw a line in the current window with GetLineFromUser and prints its end points.

Waits for two clicks in Archicad; Escape returns cancelled. The optional snapFilter makes the
cursor snap only to points (end points, vertices, hotspots), not to edges, and never to hatches.
The optional constraint keeps the end point on the horizontal line through the start point
(angle 0 measured from the last clicked point), so the line is always horizontal.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetLineFromUser', {
    'startPrompt': 'Click the start point of the line',
    'endPrompt': 'Click the end point of the horizontal line',
    'snapFilter': {'base': 'Nodes', 'exclude': ['Hatch']},
    'constraint': {'type': 'Angle', 'value': 0.0, 'fromLastPoint': True}
}))
