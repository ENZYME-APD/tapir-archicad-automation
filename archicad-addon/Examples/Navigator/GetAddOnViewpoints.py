"""Prints the type, ID, name and data of the add-on viewpoint roots with GetAddOnViewpoints.

Requires Archicad 27 or later.
"""
import aclib

roots = aclib.RunTapirCommand ('GetAddOnViewpointRoots')['viewpointIds']
if not roots:
    raise SystemExit ('No add-on viewpoint in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnViewpoints', {'viewpointIds': roots}))
