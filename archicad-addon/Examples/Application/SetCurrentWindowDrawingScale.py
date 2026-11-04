"""Sets the drawing scale of the current window to 1:50 with SetCurrentWindowDrawingScale, then restores it.

Needs a window other than a layout (layouts are always 1:1).
"""
import aclib

originalScale = aclib.RunTapirCommand ('GetCurrentWindowDrawingScale')['scale']

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentWindowDrawingScale', {'scale': 50}))
aclib.RunTapirCommand ('GetCurrentWindowDrawingScale')

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentWindowDrawingScale', {'scale': originalScale, 'rebuild': True}))
