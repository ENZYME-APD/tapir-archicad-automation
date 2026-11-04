"""Quits the running Archicad with QuitArchicad.

Quits Archicad. Save the project first.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('QuitArchicad'))
