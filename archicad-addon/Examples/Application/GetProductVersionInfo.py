"""Prints the build number, language, product name and version of the running Archicad with GetProductVersionInfo.

Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProductVersionInfo'))
