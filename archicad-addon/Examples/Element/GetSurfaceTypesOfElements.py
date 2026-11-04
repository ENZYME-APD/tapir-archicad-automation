"""Reads the surface type of every polygon of a wall's 3D model with GetSurfaceTypesOfElements.

Needs at least one Wall. Requires Archicad 28 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetSurfaceTypesOfElements', {'elements': walls}))
