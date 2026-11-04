"""Lists the elements drawn in the first section, each with the element it was generated from, with GetSectionElements.

Needs at least one Section.
"""
import aclib

sections = aclib.RunTapirCommand ('GetDatabases', {'types': ['Section']})['databases']
if not sections:
    raise SystemExit ('No section in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetSectionElements', {'databases': [{'databaseId': sections[0]['databaseId']}]}))
