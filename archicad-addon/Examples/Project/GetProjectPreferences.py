"""Prints the working units, the reference levels, the floor plan cut plane, the legacy settings, the calculation rules and the temporary folder with GetProjectPreferences.

Without preferenceTypes every page is returned. ImagingAndCalculation is the Work Environment page of the same name.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectPreferences', {'preferenceTypes': ['WorkingUnits', 'ReferenceLevels', 'FloorPlanCutPlane', 'Legacy', 'CalculationRules', 'DataSafety']}))
