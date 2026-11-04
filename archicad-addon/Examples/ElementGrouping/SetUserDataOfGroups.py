"""Groups the first two Walls, stores a key-value pair on the group with SetUserDataOfGroups and reads it back.

The script ungroups the walls again at the end. Needs at least two Walls that are not grouped yet.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Needs at least two walls.')
groups = aclib.RunTapirCommand ('CreateGroups', {'elementGroups': [{'elements': walls}]})['groupGuids']
groups = [g for g in groups if 'groupId' in g]

aclib.PrintJson (aclib.RunTapirCommand ('SetUserDataOfGroups', {
    'groups': groups,
    'pairs': [{'key': 'tapirExample.note', 'value': 'Grouped by a Tapir example'}]
}))
aclib.RunTapirCommand ('GetUserDataOfGroups', {'groups': groups})

aclib.RunTapirCommand ('UngroupElements', {'groups': groups})
