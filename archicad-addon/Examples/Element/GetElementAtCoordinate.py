"""Finds the Slab at a point of the floor plan, the way a click there would, with GetElementAtCoordinate.

The point is the average of the first slab's outline vertices. Needs at least one Slab.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements'][:1]
if not slabs:
    raise SystemExit ('No slab in the project.')

outline = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': slabs, 'fields': ['details']})['detailsOfElements'][0]['details']['polygonOutline']
point = {'x': sum (c['x'] for c in outline) / len (outline), 'y': sum (c['y'] for c in outline) / len (outline)}

aclib.PrintJson (aclib.RunTapirCommand ('GetElementAtCoordinate', {
    'elementType': 'Slab',
    'coordinate': point,
    'filters': ['IsVisibleByLayer']
}))
