"""Lists every graphic override combination of the project with the rules it contains with GetGraphicOverrideCombinations.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGraphicOverrideCombinations'))
