"""Sets the generation settings of the current Structural Analytical Model to no rules with SetAnalyticalGenerationSettingsToNoRule.

Changes the project's analytical model generation settings and does not restore them. Requires Archicad 27 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SetAnalyticalGenerationSettingsToNoRule', {}))
