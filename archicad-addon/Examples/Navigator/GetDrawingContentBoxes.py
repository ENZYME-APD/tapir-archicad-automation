"""Prints the full content box of every Drawing placed on the layouts, with GetDrawingContentBoxes.

Needs at least one Drawing on a layout. Requires Archicad 27 or later.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
drawings = aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Drawing',
    'databases': [{'databaseId': layout['databaseId']} for layout in layouts]
})['elements']
if not drawings:
    raise SystemExit ('No Drawing on the layouts.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDrawingContentBoxes', {'elements': drawings}))
