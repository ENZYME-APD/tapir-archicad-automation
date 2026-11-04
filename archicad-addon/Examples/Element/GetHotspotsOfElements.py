"""Prints the 2D hotspots of the first object with GetHotspotsOfElements.

Needs at least one Object.
"""
import aclib

objects = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Object'})['elements'][:1]
if not objects:
    raise SystemExit ('No object in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetHotspotsOfElements', {'elements': objects}))
