"""Prints the project location and the survey point (position, geo-referencing, visibility) with GetGeoLocation."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetGeoLocation'))
