"""Gets the top level group of each group of the Walls with GetMainGroups.

Needs at least one grouped Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']
groups = aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': walls}, debug = False)['groupGuids'] if walls else []
groups = list ({g['groupId']['guid']: g for g in groups if 'groupId' in g}.values ())
if not groups:
    raise SystemExit ('No grouped wall in the project.')

aclib.RunTapirCommand ('GetMainGroups', {'groups': groups})
