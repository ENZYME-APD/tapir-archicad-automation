"""Prints the dimension standards of the project with the unit and rounding of each dimension type, with GetDimensionStandards.

Pass a name to get only that standard. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetDimensionStandards', {}))
