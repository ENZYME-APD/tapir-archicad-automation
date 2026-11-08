"""Asks the user to click a point in the current window with GetPointFromUser and prints it.

Waits for a click in Archicad; Escape cancels the input and the command fails. The optional
snapFilter makes the cursor snap only to the end points and reference lines of walls.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPointFromUser', {
    'prompt': 'Click a point on a wall',
    'snapFilter': {'base': 'None', 'include': ['Wall', 'WallOn']}
}))
