"""Prints the drawing scale of the current window (100.0 for 1:100) with GetCurrentWindowDrawingScale."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCurrentWindowDrawingScale'))
