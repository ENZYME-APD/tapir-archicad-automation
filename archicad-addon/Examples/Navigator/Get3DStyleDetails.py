"""Prints the settings of the current 3D style and of every other 3D style with Get3DStyleDetails.

Requires Archicad 27 or later.
"""
import aclib

# Without names the current 3D style is returned.
aclib.PrintJson (aclib.RunTapirCommand ('Get3DStyleDetails'))

styleNames = aclib.RunTapirCommand ('Get3DStyles')['styleNames']
aclib.PrintJson (aclib.RunTapirCommand ('Get3DStyleDetails', {'names': styleNames}))
