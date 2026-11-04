"""Prints the name, path and Teamwork state of the open project with GetProjectInfo."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectInfo'))
