"""Classifies the Wall tool default with the first item of the first classification system with SetClassificationsOfElementDefaults, then restores it.

New walls get the tool default's classification. Needs a classification system with an item.
"""
import aclib

systemId = aclib.RunTapirCommand ('GetAllClassificationSystems', debug = False)['classificationSystems'][0]['classificationSystemId']
itemId = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']['classificationItemId']

def ReadClassification ():
    return aclib.RunTapirCommand ('GetClassificationsOfElementDefaults', {
        'elementTypes': ['Wall'],
        'classificationSystemIds': [{'classificationSystemId': systemId}]
    }, debug = False)['elementDefaultClassifications'][0]['classificationIds'][0]['classificationId']

oldClassification = ReadClassification ()

aclib.RunTapirCommand ('SetClassificationsOfElementDefaults', {
    'elementDefaultClassifications': [{
        'elementType': 'Wall',
        'classificationId': {'classificationSystemId': systemId, 'classificationItemId': itemId}
    }]
})
print ('Classification after the change:', ReadClassification ())

aclib.RunTapirCommand ('SetClassificationsOfElementDefaults', {
    'elementDefaultClassifications': [{'elementType': 'Wall', 'classificationId': oldClassification}]
})
