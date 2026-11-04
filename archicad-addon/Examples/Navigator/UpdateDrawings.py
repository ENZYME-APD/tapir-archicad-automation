"""Updates every Drawing placed on the layouts of the project with UpdateDrawings.

Needs at least one Drawing on a layout.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
drawings = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Drawing',
    'databases': [{'databaseId': layout['databaseId']} for layout in layouts]
})['elements']
if not drawings:
    raise SystemExit ('No Drawing on the layouts.')

aclib.PrintJson (aclib.RunTapirCommand ('UpdateDrawings', {'elements': drawings}))
