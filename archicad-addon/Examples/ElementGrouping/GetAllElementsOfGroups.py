"""Lists all elements of the groups of the Walls, nested groups included, with GetAllElementsOfGroups.

Needs at least one grouped Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
groups = aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': walls})['groupGuids'] if walls else []
groups = list ({g['groupId']['guid']: g for g in groups if 'groupId' in g}.values ())
if not groups:
    raise SystemExit ('No grouped wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetAllElementsOfGroups', {'groups': groups}))
