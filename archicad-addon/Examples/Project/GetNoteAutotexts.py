"""Prints the autotexts available in the notes of dimensions, with GetNoteAutotexts.

Requires Archicad 26 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetNoteAutotexts', {'noteOwnerType': 'Dimension'}))
