"""Checks the loaded libraries with CheckLibraries and prints the error the Archicad API reports, if any."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CheckLibraries'))
