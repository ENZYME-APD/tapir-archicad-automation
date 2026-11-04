"""Gets with GetTeamworkRightsOfObjects whether the current user may create, delete and modify the first three walls.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.RunTapirCommand ('GetTeamworkRightsOfObjects', {
    'objectIds': [wall['elementId'] for wall in walls]
})
