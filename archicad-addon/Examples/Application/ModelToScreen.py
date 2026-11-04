"""Converts two model points to pixel positions of the current view window with ModelToScreen.

The pixels depend on the window's size, scale and zoom. Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ModelToScreen', {
    'coordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 5.0, 'y': 3.0, 'z': 0.0}
    ]
}))
