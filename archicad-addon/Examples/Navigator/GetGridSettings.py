"""Prints the grid and snap grid settings of the current window's database with GetGridSettings.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGridSettings'))
