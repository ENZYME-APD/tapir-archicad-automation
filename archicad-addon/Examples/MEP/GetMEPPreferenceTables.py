"""Prints the pipe preference tables of the project, with the elbow, branch and transition tables, using GetMEPPreferenceTables.

The angle set and the transition tables are returned from Archicad 29. Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPPreferenceTables', {
    'domain': 'Piping',
    'includeFittingTables': True
}))
