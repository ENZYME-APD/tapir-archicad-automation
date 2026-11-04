"""Reads the building material of each body of the 3D model of the first walls with GetBodyMaterialsOfElements.

A composite wall has one body per skin. Needs at least one Wall. Requires Archicad 28 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetBodyMaterialsOfElements', {'elements': walls})
