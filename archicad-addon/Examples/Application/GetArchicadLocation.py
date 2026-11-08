"""Prints where the running Archicad executable is, with GetArchicadLocation."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetArchicadLocation'))
