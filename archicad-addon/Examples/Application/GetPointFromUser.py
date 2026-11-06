"""Asks the user to click a point in the current window with GetPointFromUser and prints it.

Waits for a click in Archicad; Escape cancels the input and the command fails.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPointFromUser', {'prompt': 'Click a point'}))
