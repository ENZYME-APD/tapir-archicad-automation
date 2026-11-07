"""Deletes the trace reference (ghost) of the first section with DeleteGhostRecords.

Needs at least one Section; one without a trace reference gets a failed result. Not undoable.
"""
import aclib

sections = aclib.RunTapirCommand ('GetDatabases', {'types': ['Section']})['databases'][:1]
if not sections:
    raise SystemExit ('No section in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('DeleteGhostRecords', {
    'databases': [{'databaseId': sections[0]['databaseId']}]
}))
