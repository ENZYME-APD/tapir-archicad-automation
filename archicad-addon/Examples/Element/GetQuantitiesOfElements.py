"""Reads the quantities Archicad calculates (volumes, surfaces, lengths ...) of the first walls with GetQuantitiesOfElements.

Openings smaller than 0.5 m2 are not subtracted. Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetQuantitiesOfElements', {
    'elements': walls,
    'minOpeningSize': 0.5
}))
