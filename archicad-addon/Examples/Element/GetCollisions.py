"""Finds the walls that collide with columns or beams with GetCollisions.

Needs Walls and Columns or Beams.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']
others = []
for elementType in ['Column', 'Beam']:
    others += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType}, debug = False)['elements']
if not walls or not others:
    raise SystemExit ('No walls, or no columns or beams in the project.')

aclib.RunTapirCommand ('GetCollisions', {
    'elementsGroup1': walls,
    'elementsGroup2': others,
    'settings': {
        'volumeTolerance': 0.001,
        'performSurfaceCheck': False,
        'surfaceTolerance': 0.001
    }
})
