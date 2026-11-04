"""Zooms the current window to the first three walls with ZoomToElements, keeping the selection as it was.

Needs at least one Wall on the current story.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall', 'filters': ['OnActualFloor']})['elements'][:3]
if not walls:
    raise SystemExit ('No wall on the current story.')

aclib.PrintJson (aclib.RunTapirCommand ('ZoomToElements', {'elements': walls, 'keepSelection': True}))
