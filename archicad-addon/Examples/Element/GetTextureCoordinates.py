"""Reads the texture coordinates (u, v) of the faces of a wall's 3D model with GetTextureCoordinates.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetTextureCoordinates', {'elements': walls}))
