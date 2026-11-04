"""Groups the first two Walls with CreateGroups, then ungroups them again.

Needs at least two Walls that are not grouped yet.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')

groups = aclib.RunTapirCommand ('CreateGroups', {'elementGroups': [{'elements': walls}]})['groupGuids']
aclib.PrintJson (groups)

aclib.RunTapirCommand ('UngroupElements', {'groups': [g for g in groups if 'groupId' in g]})
