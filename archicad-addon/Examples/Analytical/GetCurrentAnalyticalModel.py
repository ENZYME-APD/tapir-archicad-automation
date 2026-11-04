"""Prints the id of the Structural Analytical Model of the current view with GetCurrentAnalyticalModel.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCurrentAnalyticalModel', {}))
