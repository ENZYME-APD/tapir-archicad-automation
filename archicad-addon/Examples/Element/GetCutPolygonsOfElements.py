"""Cuts the first walls with a horizontal plane 1 m above project zero with GetCutPolygonsOfElements.

Returns the building material, cut area and cut polygons of each body. Needs at least one Wall. On Archicad 25-27
the building materials are left out.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetCutPolygonsOfElements', {
    'elements': walls,
    'cutPlane': {
        'basePoint': {'x': 0.0, 'y': 0.0, 'z': 1.0},
        'axisX': {'x': 1.0, 'y': 0.0, 'z': 0.0},
        'axisY': {'x': 0.0, 'y': 1.0, 'z': 0.0},
        'axisZ': {'x': 0.0, 'y': 0.0, 'z': 1.0}
    }
}))
