"""Reads the location, rotation angle and scaling of the first placed objects, windows and doors with GetLibPartTransformations.

Needs at least one Object, Window or Door.
"""
import aclib

elements = []
for elementType in ['Object', 'Window', 'Door']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']
if not elements:
    raise SystemExit ('No object, window or door in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibPartTransformations', {'elements': elements[:5]}))
