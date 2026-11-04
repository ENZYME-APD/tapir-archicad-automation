"""Prints the Partial Structure Display of the current window with GetStructureDisplay.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetStructureDisplay'))
