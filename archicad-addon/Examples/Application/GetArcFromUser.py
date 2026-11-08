"""Asks the user to draw an arc in the current window with GetArcFromUser and prints its center, radius and angles.

Waits for three clicks in Archicad (center, start point, end point); Escape returns cancelled.
The optional snapFilter makes the center and the start point snap only to points (end points,
vertices, hotspots), not to edges, and never to hatches; the end point snaps as usual. The
optional constraint keeps every click at the height 0 (type Z), which matters in the 3D window.
"""
import aclib

aclib.raiseOnError = True
aclib.RequireTapir ('1.8.1')
aclib.PrintJson (aclib.RunTapirCommand ('GetArcFromUser', {
    'centerPrompt': 'Click the center of the arc',
    'startPrompt': 'Click the start point of the arc',
    'endPrompt': 'Click the end point of the arc',
    'snapFilter': {'base': 'Nodes', 'exclude': ['Hatch']},
    'constraint': {'type': 'Z', 'value': 0.0}
}))
