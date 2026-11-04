"""Prints the type and the database id of the current window with GetCurrentWindowType."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCurrentWindowType'))
