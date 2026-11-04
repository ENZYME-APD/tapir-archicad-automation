"""Prints the drawing numbering grid of the first layout with GetDrawingNumberingGridLines.

Opens the first layout, as the command reads the grid of the current window. Only a layout
with grid-based drawing numbering has such a grid.
"""
import aclib

layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']})['databases']
if not layouts:
    raise SystemExit ('No layout in the project.')

aclib.RunTapirCommand ('ChangeWindow', {'databaseId': layouts[0]['databaseId']})
aclib.PrintJson (aclib.RunTapirCommand ('GetDrawingNumberingGridLines'))
