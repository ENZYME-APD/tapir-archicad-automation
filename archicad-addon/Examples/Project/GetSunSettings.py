"""Prints the project's date and time, the sun position for them and the sun and ambient light settings, with GetSunSettings."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetSunSettings'))
