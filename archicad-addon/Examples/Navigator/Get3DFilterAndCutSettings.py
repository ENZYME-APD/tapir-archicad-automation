"""Prints the 'Filter and Cut Elements in 3D' settings of the 3D window with Get3DFilterAndCutSettings.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('Get3DFilterAndCutSettings'))
