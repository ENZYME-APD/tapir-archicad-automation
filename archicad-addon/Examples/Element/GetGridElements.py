"""Prints the position, direction, length and library part of every grid element with GetGridElements.

Set includeParameters to True to get their GDL parameters as well. Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGridElements', {'includeParameters': False}))
