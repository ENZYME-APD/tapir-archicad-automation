"""Asks the user to draw a polygon in the current window with GetPolygonFromUser and prints its points and arcs.

Waits for the polygon input in Archicad (a double click or Enter finishes it); Escape returns cancelled.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPolygonFromUser', {
    'startPrompt': 'Click the first corner of the polygon',
    'prompt': 'Click the other corners, double click to finish',
    'method': 'General'
}))
