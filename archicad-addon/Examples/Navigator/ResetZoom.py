"""Fits everything in the current window, then steps back to the previous zoom with ResetZoom, like Zoom Back."""
import aclib

aclib.RunTapirCommand ('FitInWindow')

aclib.RunTapirCommand ('ResetZoom', {'numberOfStepsBack': 1})
