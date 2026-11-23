"""Prints the working units, the reference levels, the floor plan cut plane, the legacy settings and the temporary folder with GetProjectPreferences.

Without preferenceTypes every page is returned. ImagingAndCalculation is the Work Environment page of the same name.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'ReferenceLevels', 'FloorPlanCutPlane', 'Legacy', 'DataSafety']}))
