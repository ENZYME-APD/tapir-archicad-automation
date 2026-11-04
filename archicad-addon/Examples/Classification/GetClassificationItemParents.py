"""Gets the parent item and the classification system of a classification item with GetClassificationItemParents.

Uses the first child of the first item of the first classification system, or that item when it has no child.
"""
import aclib

systemId = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems'][0]['classificationSystemId']
item = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']
if item.get ('children'):
    item = item['children'][0]['classificationItem']

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationItemParents', {
    'classificationItemIds': [{'classificationItemId': item['classificationItemId']}]
}))
