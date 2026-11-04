"""Lists the elements that are classified like the first Wall with GetElementsByClassification.

Needs a classified Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
systems = aclib.RunTapirCommand ('GetAllClassificationSystems', debug = False)['classificationSystems']
classificationIds = aclib.RunTapirCommand ('GetClassificationsOfElements', {
    'elements': walls,
    'classificationSystemIds': [{'classificationSystemId': system['classificationSystemId']} for system in systems]
}, debug = False)['elementClassifications'][0]['classificationIds']
itemIds = [{'classificationItemId': c['classificationId']['classificationItemId']} for c in classificationIds if 'classificationItemId' in c.get ('classificationId', {})]
if not itemIds:
    raise SystemExit ('The wall is not classified.')

aclib.RunTapirCommand ('GetElementsByClassification', {'classificationItemIds': itemIds})
