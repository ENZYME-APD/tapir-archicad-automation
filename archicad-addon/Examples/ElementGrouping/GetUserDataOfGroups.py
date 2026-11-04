"""Reads the user data add-ons attached to the groups of the Walls with GetUserDataOfGroups.

A group without user data gets an error item. Needs at least one grouped Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']
groups = aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': walls}, debug = False)['groupGuids'] if walls else []
groups = list ({g['groupId']['guid']: g for g in groups if 'groupId' in g}.values ())
if not groups:
    raise SystemExit ('No grouped wall in the project.')

aclib.RunTapirCommand ('GetUserDataOfGroups', {'groups': groups})
