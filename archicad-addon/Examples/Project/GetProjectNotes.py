"""Prints the Project Notes (client, company, address, architect, status, notes...) with GetProjectNotes."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectNotes'))
