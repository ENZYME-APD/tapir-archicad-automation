"""Shows only the core of the structures in the current window with SetStructureDisplay, then restores the original display.

Requires Archicad 27 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetStructureDisplay')

aclib.PrintJson (aclib.RunTapirCommand ('SetStructureDisplay', {'structureDisplay': 'CoreOnly'}))
aclib.RunTapirCommand ('GetStructureDisplay')

aclib.PrintJson (aclib.RunTapirCommand ('SetStructureDisplay', {'structureDisplay': original['structureDisplay']}))
