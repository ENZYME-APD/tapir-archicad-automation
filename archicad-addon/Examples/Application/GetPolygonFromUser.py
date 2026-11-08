"""Asks the user to draw a polygon in the current window with GetPolygonFromUser and prints its points and arcs.

Waits for the polygon input in Archicad (a double click or Enter finishes it); Escape returns cancelled.
The optional snapFilter makes the first corner snap only to the corners of zones; the other corners
snap as usual.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPolygonFromUser', {
    'startPrompt': 'Click the first corner of the polygon on a zone corner',
    'prompt': 'Click the other corners, double click to finish',
    'method': 'General',
    'snapFilter': {'include': ['Zone']}
}))
