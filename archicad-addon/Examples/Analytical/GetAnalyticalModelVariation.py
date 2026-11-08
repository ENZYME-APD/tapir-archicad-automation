"""Prints the variation of the Structural Analytical Model of the current view with GetAnalyticalModelVariation.

Without 'analyticalModelId' the model of the current view is used, from Archicad 27; before that pass the model.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalModelVariation', {}))
