"""Prints the hotlink nodes of the project (source file, name, type, placed instances and children) with GetHotlinks."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetHotlinks'))
