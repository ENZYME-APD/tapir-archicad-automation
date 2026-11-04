"""Classifies the first Building Material with the first item of the first classification system with SetClassificationsOfAttributes, then restores it.

Needs a classification system with an item.
"""
import aclib

buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]
attributeIds = [{'attributeId': buildingMaterial['attributeId']}]
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems'][0]['classificationSystemId']
itemId = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']['classificationItemId']

def ReadClassification ():
    return aclib.RunTapirCommand ('GetClassificationOfAttributesInSystem', {
        'attributeIds': attributeIds,
        'classificationSystemId': systemId
    })['attributeClassifications'][0]['classificationId']

oldClassification = ReadClassification ()

aclib.PrintJson (aclib.RunTapirCommand ('SetClassificationsOfAttributes', {
    'attributeClassifications': [{
        'attributeId': buildingMaterial['attributeId'],
        'classificationId': {'classificationSystemId': systemId, 'classificationItemId': itemId}
    }]
}))
print ('Classification after the change:', ReadClassification ())

aclib.PrintJson (aclib.RunTapirCommand ('SetClassificationsOfAttributes', {
    'attributeClassifications': [{'attributeId': buildingMaterial['attributeId'], 'classificationId': oldClassification}]
}))
