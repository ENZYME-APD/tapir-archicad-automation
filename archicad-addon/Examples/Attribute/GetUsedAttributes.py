"""Gets the attributes in use in the project with GetUsedAttributes.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetUsedAttributes'))
