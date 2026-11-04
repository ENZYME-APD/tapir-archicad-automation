"""Prints the display names of two autotext keys with GetAutoTextName."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAutoTextName', {'keys': ['PROJECTNAME', 'PROJECTNUMBER']}))
