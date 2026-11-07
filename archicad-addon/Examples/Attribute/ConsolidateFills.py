"""Runs Archicad's Fill Consolidation on the current window with ConsolidateFills.

Changes the fills of the current window and may show Archicad's dialog. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ConsolidateFills'))
