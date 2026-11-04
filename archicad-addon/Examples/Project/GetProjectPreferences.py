"""Prints the working units and the zone settings of Options > Project Preferences with GetProjectPreferences.

Without preferenceTypes every page is returned.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'Zones']}))
