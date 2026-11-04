"""Asks the user to click a wall or a column with GetElementFromUser and prints its id, type and the clicked point.

Waits for a click in Archicad; Escape returns cancelled.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetElementFromUser', {
    'prompt': 'Click a wall or a column',
    'elementTypes': ['Wall', 'Column']
}))
