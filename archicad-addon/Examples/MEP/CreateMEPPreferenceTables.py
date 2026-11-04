"""Adds an empty pipe preference table named "Tapir example" with CreateMEPPreferenceTables.

Every run adds a new table. Only for Archicad 27; from Archicad 28 use ModifyMEPPreferenceTables.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPPreferenceTables', {
    'domain': 'Piping',
    'tables': [{'name': 'Tapir example'}]
}))
