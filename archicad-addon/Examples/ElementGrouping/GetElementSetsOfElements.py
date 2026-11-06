"""Gets the element sets that contain each of the first three Walls with GetElementSetsOfElements.

Only element sets created by Tapir are found. Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetElementSetsOfElements', {'elements': walls}))
