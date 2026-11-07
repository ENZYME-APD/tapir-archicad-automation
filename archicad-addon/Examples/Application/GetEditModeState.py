"""Prints whether Archicad is in curtain wall, stair, beam or column editing mode with GetEditModeState.

Requires Archicad 27 or later (the overall inEditMode flag needs Archicad 29).
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetEditModeState'))
