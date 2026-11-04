"""Prints the virtual origin offset and the user origin of the current database with GetCoordinateOrigins."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetCoordinateOrigins'))
