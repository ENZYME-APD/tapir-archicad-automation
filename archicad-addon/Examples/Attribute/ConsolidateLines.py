"""Runs Archicad's Line Consolidation on the current window with ConsolidateLines.

Changes the lines of the current window and may show Archicad's dialog. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ConsolidateLines'))
