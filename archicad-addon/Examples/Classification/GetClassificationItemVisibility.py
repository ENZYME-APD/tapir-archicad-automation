"""Tells whether the first item of the first classification system is offered for the first Wall, the Wall tool default and the first Building Material with GetClassificationItemVisibility.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
buildingMaterial = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributes'][0]
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems'][0]['classificationSystemId']
item = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationItemVisibility', {
    'targets': [
        {'elementId': walls[0]['elementId']},
        {'elementType': 'Wall'},
        {'attributeId': buildingMaterial['attributeId']}
    ],
    'classificationItemIds': [{'classificationItemId': item['classificationItemId']}]
}))
