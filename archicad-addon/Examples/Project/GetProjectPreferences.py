"""Prints the working units, the reference levels and the floor plan cut plane with GetProjectPreferences.

Without preferenceTypes every page is returned.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'ReferenceLevels', 'FloorPlanCutPlane']}))
