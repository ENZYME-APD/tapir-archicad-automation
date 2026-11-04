"""Reads the texture coordinates (u, v) of the faces of a wall's 3D model with GetTextureCoordinates.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetTextureCoordinates', {'elements': walls})
