"""Prints the library part, size and GDL parameters a new grid element gets from the Grid Tool defaults, with GetGridElementDefaults.

Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGridElementDefaults', {}))
