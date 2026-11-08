"""Gets the date GDL scripts see with GetGDLDate, as year, month, day, hour, minute and second."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGDLDate'))
