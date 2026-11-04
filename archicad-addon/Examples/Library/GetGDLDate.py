"""Gets the date GDL scripts see with GetGDLDate, as the integer value the Archicad API returns."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGDLDate'))
