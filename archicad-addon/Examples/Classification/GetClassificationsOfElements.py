"""Reads the classification of the first three Walls in every classification system with GetClassificationsOfElements.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')
systems = aclib.RunTapirCommand ('GetAllClassificationSystems', debug = False)['classificationSystems']

aclib.RunTapirCommand ('GetClassificationsOfElements', {
    'elements': walls,
    'classificationSystemIds': [{'classificationSystemId': system['classificationSystemId']} for system in systems]
})
