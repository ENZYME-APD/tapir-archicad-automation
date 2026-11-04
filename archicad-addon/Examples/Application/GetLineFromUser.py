"""Asks the user to draw a line in the current window with GetLineFromUser and prints its end points.

Waits for two clicks in Archicad; Escape returns cancelled.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetLineFromUser', {
    'startPrompt': 'Click the start point of the line',
    'endPrompt': 'Click the end point of the line'
}))
