"""Zooms the current window to the walls of the project with FitInWindow, then fits everything in the window."""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if walls:
    aclib.PrintJson (aclib.RunTapirCommand ('FitInWindow', {'elements': walls}))

# Without elements, everything is fitted in the window.
aclib.PrintJson (aclib.RunTapirCommand ('FitInWindow'))
