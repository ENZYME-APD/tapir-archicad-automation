"""Reads the 3D geometry (vertices and faces of the solid bodies) of a wall with Get3DGeometryOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')

geometry = aclib.RunTapirCommand ('Get3DGeometryOfElements', {'elements': walls})['geometries3D'][0]
for body in geometry.get ('bodies', []):
    print ('Body with {} vertices and {} faces'.format (len (body['vertices']), len (body['polygons'])))
