"""Rebuilds the current database with RebuildCurrentDatabase.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('RebuildCurrentDatabase'))
