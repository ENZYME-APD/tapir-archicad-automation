"""Prints the size, margins, numbering, master layout and custom data of every layout with GetLayoutSettings."""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not layouts:
    raise SystemExit ('No layout in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLayoutSettings', {
    'layoutDatabaseIds': [{'databaseId': layout['databaseId']} for layout in layouts]
}))
