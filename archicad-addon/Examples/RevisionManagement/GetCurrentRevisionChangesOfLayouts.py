"""Lists the changes of the last revision of every layout with GetCurrentRevisionChangesOfLayouts.

Needs at least one layout.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not layouts:
    raise SystemExit ('No layout in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetCurrentRevisionChangesOfLayouts', {
    'layoutDatabaseIds': [{'databaseId': layout['databaseId']} for layout in layouts]
}))
