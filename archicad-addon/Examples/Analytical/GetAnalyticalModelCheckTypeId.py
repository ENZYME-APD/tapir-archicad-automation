"""Prints the type id of the Structural Analytical Model check with GetAnalyticalModelCheckTypeId.

Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalModelCheckTypeId', {}))
