"""Asks the user to click a wall or a column with GetElementFromUser and prints its id, type and the clicked point.

Waits for a click in Archicad; Escape returns cancelled. The optional snapFilter makes the cursor
snap only to the points and lines of walls and columns.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetElementFromUser', {
    'prompt': 'Click a wall or a column',
    'elementTypes': ['Wall', 'Column'],
    'snapFilter': {'include': ['Wall', 'WallOn', 'Column', 'ColumnOn']}
}))
