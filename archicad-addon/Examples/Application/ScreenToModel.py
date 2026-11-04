"""Converts two pixel positions of the current view window to model coordinates with ScreenToModel.

The result depends on the window's size, scale and zoom.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ScreenToModel', {
    'screenPoints': [
        {'h': 0, 'v': 0},
        {'h': 200, 'v': 100}
    ]
}))
