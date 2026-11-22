"""Closes the open project with CloseProject.

Closes the project. Save it first. When Archicad keeps the project open (as it does
with its last open project), the command fails and says so instead of answering success.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CloseProject'))
