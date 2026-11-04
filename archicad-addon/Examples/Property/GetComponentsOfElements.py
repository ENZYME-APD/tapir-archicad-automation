"""Prints the components (building material skins or profile parts) of the first walls with GetComponentsOfElements.

Needs at least one Wall; a basic wall has a single component.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetComponentsOfElements', {'elements': walls}))
