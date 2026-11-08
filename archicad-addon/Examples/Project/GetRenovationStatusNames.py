"""Prints the localized names of the renovation statuses Existing, New and Demolished with GetRenovationStatusNames.

Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetRenovationStatusNames'))
