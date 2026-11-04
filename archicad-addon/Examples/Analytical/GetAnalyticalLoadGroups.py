"""Prints the structural analysis load groups of the project with GetAnalyticalLoadGroups."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAnalyticalLoadGroups', {}))
