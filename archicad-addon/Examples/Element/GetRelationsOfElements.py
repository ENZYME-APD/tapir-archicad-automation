"""Reads the relations of the first walls to other walls (connected endpoints and reference lines) with GetRelationsOfElements.

Needs at least one Wall. Requires Archicad 26 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetRelationsOfElements', {
    'elements': walls,
    'otherElementType': 'Wall'
}))
