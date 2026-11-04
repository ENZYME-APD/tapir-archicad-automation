"""Classifies the first Wall with the first item of the first classification system with SetClassificationsOfElements, then restores its classification.

Needs at least one Wall and a classification system with an item.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems'][0]['classificationSystemId']
itemId = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']['classificationItemId']

def ReadClassification ():
    return aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements': walls,
        'classificationSystemIds': [{'classificationSystemId': systemId}]
    })['elementClassifications'][0]['classificationIds'][0]['classificationId']

oldClassification = ReadClassification ()

aclib.PrintJson (aclib.RunTapirCommand ('SetClassificationsOfElements', {
    'elementClassifications': [{
        'elementId': walls[0]['elementId'],
        'classificationId': {'classificationSystemId': systemId, 'classificationItemId': itemId}
    }]
}))
print ('Classification after the change:', ReadClassification ())

aclib.PrintJson (aclib.RunTapirCommand ('SetClassificationsOfElements', {
    'elementClassifications': [{'elementId': walls[0]['elementId'], 'classificationId': oldClassification}]
}))
