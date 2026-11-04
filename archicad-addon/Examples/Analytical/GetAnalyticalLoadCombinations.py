"""Prints the structural analysis load combinations of the project and their load factors with GetAnalyticalLoadCombinations."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalLoadCombinations', {}))
