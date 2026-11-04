"""Reads the snap points (hotspots) of a wall with GetSnapPointsOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetSnapPointsOfElements', {'elements': walls})
