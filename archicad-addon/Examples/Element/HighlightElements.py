"""Highlights the walls in red, with the other elements in transparent blue and wireframe in 3D, with HighlightElements.

Needs at least one Wall. The highlight is removed after 5 seconds.
"""
import time
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('HighlightElements', {
    'elements': walls,
    'highlightedColors': [[255, 0, 0, 255] for wall in walls],
    'wireframe3D': True,
    'nonHighlightedColor': [0, 0, 255, 128]
}))

time.sleep (5)

# An empty elements array removes the highlight.
aclib.PrintJson (aclib.RunTapirCommand ('HighlightElements', {'elements': [], 'highlightedColors': []}))
