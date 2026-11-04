"""Prints the calculation units of the project (length, area, volume, angle) with GetCalculationUnits."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCalculationUnits'))
