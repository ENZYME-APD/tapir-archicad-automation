"""Gets the Structural Analytical Model of the variation of the current view with GetAnalyticalModel.

Reading the variation of the current view requires Archicad 27 or later.
"""
import aclib

variation = aclib.RunTapirCommand ('GetAnalyticalModelVariation', {})['variation']

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalModel', {
    'variation': variation
}))
