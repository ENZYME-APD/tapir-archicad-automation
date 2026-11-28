"""Reads the terrain elevations at two boundary points of the first mesh in the project.

Each reported elevation is in metres relative to project zero and identifies the element that provides it.
"""
import aclib

meshes = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Mesh'})['elements'][:1]
if not meshes:
    raise SystemExit ('No mesh in the project.')

mesh = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': meshes, 'fields': ['details']})['detailsOfElements'][0]
outline = mesh['details']['polygonCoordinates']
points = [{'x': p['x'], 'y': p['y']} for p in outline[:2]]
aclib.PrintJson (aclib.RunTapirCommand ('GetSurfaceElevationsAtPoints', {'elements': meshes, 'points': points}))
