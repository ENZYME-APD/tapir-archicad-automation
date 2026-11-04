"""Gets the IFC assignments of the project with GetIFCAssignments: the root IFC groups and the objects assigned to each other.

Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCAssignments'))
