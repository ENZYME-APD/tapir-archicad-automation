"""Opens the first layout, goes back to the floor plan and closes the layout's window with CloseWindows.

Needs at least one Layout. Requires Archicad 27 or later.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not layouts:
    raise SystemExit ('No layout in the project.')
layoutId = layouts[0]['databaseId']

aclib.RunTapirCommand ('ChangeWindow', {'databaseId': layoutId})
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

aclib.PrintJson (aclib.RunTapirCommand ('CloseWindows', {
    'windows': [
        {'databaseId': layoutId, 'windowType': 'Layout'}
    ]
}))
