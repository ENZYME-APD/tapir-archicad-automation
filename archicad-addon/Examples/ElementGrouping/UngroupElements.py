"""Groups the first two Walls and ungroups them again with UngroupElements.

Needs at least two Walls that are not grouped yet.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')
groups = aclib.RunTapirCommand ('CreateGroups', {'elementGroups': [{'elements': walls}]})['groupGuids']

aclib.PrintJson (aclib.RunTapirCommand ('UngroupElements', {'groups': [g for g in groups if 'groupId' in g]}))
