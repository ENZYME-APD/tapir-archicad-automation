"""Reads the details (type, story, layer, geometry etc.) of the first walls with GetDetailsOfElements.

Needs at least one Wall. The second call asks only for the fields it needs, which is faster.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls}))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': walls, 'fields': ['id', 'floorIndex', 'layerIndex']}))
