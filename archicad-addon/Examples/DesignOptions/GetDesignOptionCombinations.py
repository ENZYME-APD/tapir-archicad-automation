"""Lists the design option combinations of the project and their active options with GetDesignOptionCombinations.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetDesignOptionCombinations'))
