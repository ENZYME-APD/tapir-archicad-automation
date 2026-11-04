"""Gets the Teamwork owner of the first three layouts with GetTeamworkOwnersOfDatabases.

Needs at least one Layout. The user id is 0 when nobody owns the layout.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases'][:3]
if not layouts:
    raise SystemExit ('No layout in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetTeamworkOwnersOfDatabases', {
    'databases': [{'databaseId': layout['databaseId']} for layout in layouts]
}))
