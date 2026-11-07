"""Prints the autotexts available in the notes of linear and of elevation dimensions, with GetNoteAutotexts.

For noteOwnerType 'Dimension' the optional dimensionAppearance (Linear, Cumulative, Baseline or
Elevation) chooses the dimension type; it defaults to Linear.

Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetNoteAutotexts', {'noteOwnerType': 'Dimension'}))
aclib.PrintJson (aclib.RunTapirCommand ('GetNoteAutotexts', {'noteOwnerType': 'Dimension', 'dimensionAppearance': 'Elevation'}))
