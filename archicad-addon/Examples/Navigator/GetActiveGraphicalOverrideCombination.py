"""Prints the graphic override combination applied to the current window with GetActiveGraphicalOverrideCombination.

The response is empty when no combination is applied. Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetActiveGraphicalOverrideCombination'))
