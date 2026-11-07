"""Prints the visibility settings of the Structural Analytical Model with GetAnalyticalVisibilitySettings.

Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalVisibilitySettings', {}))
