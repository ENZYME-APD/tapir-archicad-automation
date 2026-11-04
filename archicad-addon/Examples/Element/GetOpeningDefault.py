"""Prints the extrusion and floor plan settings of the Opening tool defaults with GetOpeningDefault.

Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetOpeningDefault', {}))
