"""Triangulates the plan polygon of the first slabs with TriangulatePolygonsOfElements.

Needs at least one Slab.
"""
import aclib

slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements'][:3]
if not slabs:
    raise SystemExit ('No slab in the project.')

triangulations = aclib.RunTapirCommand ('TriangulatePolygonsOfElements', {'elements': slabs})['triangulations']
for slab, triangulation in zip (slabs, triangulations):
    print ('{}: {} triangles'.format (slab['elementId']['guid'], len (triangulation.get ('triangles', []))))
