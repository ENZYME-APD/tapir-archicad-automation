"""Gets the group that directly contains each of the first three Walls with GetGroupsOfElements.

A wall that is not in a group gets an error item. Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': walls}))
