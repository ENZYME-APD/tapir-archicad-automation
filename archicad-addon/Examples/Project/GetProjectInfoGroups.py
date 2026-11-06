"""Prints the Project Info groups in their order, each with its fields, with GetProjectInfoGroups.

Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectInfoGroups'))
