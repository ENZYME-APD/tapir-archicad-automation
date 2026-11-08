"""Asks the user to draw an arc in the current window with GetArcFromUser and prints its center, radius and angles.

Waits for three clicks in Archicad (center, start point, end point); Escape returns cancelled.
"""
import aclib

aclib.raiseOnError = True
aclib.RequireTapir ('1.8.1')
aclib.PrintJson (aclib.RunTapirCommand ('GetArcFromUser', {
    'centerPrompt': 'Click the center of the arc',
    'startPrompt': 'Click the start point of the arc',
    'endPrompt': 'Click the end point of the arc'
}))
