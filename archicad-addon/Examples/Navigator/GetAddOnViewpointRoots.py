"""Lists the roots of the add-on viewpoints (Project Map items made by add-ons) with GetAddOnViewpointRoots.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnViewpointRoots'))
