"""Creates an MEP system group named "Heating" with CreateMEPSystemGroups.

Every run adds a new group; there is no command to delete it. Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPSystemGroups', {
    'systemGroupsData': [{'name': 'Heating'}]
}))
