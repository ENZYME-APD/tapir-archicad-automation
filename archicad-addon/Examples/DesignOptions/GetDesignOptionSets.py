"""Lists the design option sets of the project and their options with GetDesignOptionSets.

Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetDesignOptionSets'))
