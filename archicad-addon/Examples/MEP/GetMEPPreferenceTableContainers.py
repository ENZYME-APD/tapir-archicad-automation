"""Lists the preference tables of the pipe and duct preference table containers with GetMEPPreferenceTableContainers.

Only for Archicad 27; from Archicad 28 use GetMEPPreferenceTables.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPPreferenceTableContainers', {
    'domains': ['Piping', 'Ventilation']
}))
