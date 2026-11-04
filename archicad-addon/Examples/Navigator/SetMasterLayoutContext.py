"""Opens the first master layout and draws it in the context of the first layout with SetMasterLayoutContext.

Its autotexts then show the values of that layout. Needs a layout and a master layout.
Requires Archicad 27 or later.
"""
import aclib

databases = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout', 'MasterLayout']})['databases']
layouts = [d for d in databases if d['type'] == 'Layout']
masters = [d for d in databases if d['type'] == 'MasterLayout']
if not layouts or not masters:
    raise SystemExit ('The project needs a layout and a master layout.')

# The current window must be a master layout.
aclib.RunTapirCommand ('ChangeWindow', {'databaseId': masters[0]['databaseId']})
aclib.PrintJson (aclib.RunTapirCommand ('SetMasterLayoutContext', {'layoutDatabaseId': layouts[0]['databaseId']}))
