"""Prints the id, name and value of every Project Info field with GetProjectInfoFields."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectInfoFields'))
