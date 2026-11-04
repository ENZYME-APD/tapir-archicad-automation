"""Prints the default voltage, wiring types and load classifications of the project with GetElectricalProjectSettings.

Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetElectricalProjectSettings', {}))
