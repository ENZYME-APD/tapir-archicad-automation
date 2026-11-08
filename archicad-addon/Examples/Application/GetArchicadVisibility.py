"""Prints whether the Archicad application is shown or hidden with GetArchicadVisibility.

Windows only; on macOS it returns an error.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetArchicadVisibility'))
