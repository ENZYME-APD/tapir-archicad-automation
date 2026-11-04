"""Reads the color, width and description of pens 1 to 3 in effect in the current window with GetPens.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPens', {'indices': [1, 2, 3]}))
