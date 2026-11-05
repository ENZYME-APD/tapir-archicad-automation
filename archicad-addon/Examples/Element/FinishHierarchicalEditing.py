"""Tries to leave a hierarchical editing mode with FinishHierarchicalEditing, discarding the changes (Cancel).

accept True would keep the changes (OK). Limitation: while an editing mode is open, Archicad refuses
JSON commands with error 4001 (ongoing user input) before they run, so this call is refused then;
leave the mode in Archicad, with the button of the edit mode palette. When no editing mode is open,
it fails with "not in a hierarchical editing mode". Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('FinishHierarchicalEditing', {'accept': False}))
