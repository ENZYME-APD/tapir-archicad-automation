"""Prints whether 3D Cutaway is on and the cutting planes are shown, with Get3DCutawayStatus.

In Archicad 25-27 the 3D window may need to be open.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('Get3DCutawayStatus'))
