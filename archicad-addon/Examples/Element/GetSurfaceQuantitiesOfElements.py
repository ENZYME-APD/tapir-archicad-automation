"""Reads the exposed surface areas of the first walls, with the slabs as covering elements, with GetSurfaceQuantitiesOfElements.

Needs at least one Wall. Without coverElements every construction element covers the surfaces.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
slabs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Slab'})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('GetSurfaceQuantitiesOfElements', {
    'elements': walls,
    'coverElements': slabs
}))
