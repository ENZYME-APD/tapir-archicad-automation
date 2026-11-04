"""Prints the names of the project data stored by Tapir, with their Teamwork owners, with GetProjectDataKeys."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectDataKeys'))
