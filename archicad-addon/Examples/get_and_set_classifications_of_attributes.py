import aclib

# The first building material of the project is classified with the first (deepest) item of the
# first classification system, read back, then set to unclassified in that system again.

buildingMaterials = aclib.RunTapirCommand (
    'GetAttributesByType', {
        'attributeType': 'BuildingMaterial'
    }, debug=False)['attributes']
buildingMaterialId = buildingMaterials[0]['attributeId']
print ('Building material: {}'.format (buildingMaterials[0]['name']))

classificationSystem = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems'][0]
classificationSystemId = classificationSystem['classificationSystemId']
print ('Classification system: {}'.format (classificationSystem['name']))

classificationItem = aclib.RunCommand ('API.GetAllClassificationsInSystem', {
        'classificationSystemId': classificationSystemId
    })['classificationItems'][0]['classificationItem']
while 'children' in classificationItem and len (classificationItem['children']) > 0:
    classificationItem = classificationItem['children'][0]['classificationItem']
classificationItemId = classificationItem['classificationItemId']
print ('Classification item: {}'.format (classificationItem['id']))

aclib.RunTapirCommand (
    'GetClassificationsOfAttributes', {
        'attributeIds': [{'attributeId': buildingMaterialId}]
    })

aclib.RunTapirCommand (
    'SetClassificationsOfAttributes', {
        'attributeClassifications': [{
            'attributeId': buildingMaterialId,
            'classificationId': {
                'classificationSystemId': classificationSystemId,
                'classificationItemId': classificationItemId
            }
        }]
    })

classifications = aclib.RunTapirCommand (
    'GetClassificationsOfAttributes', {
        'attributeIds': [{'attributeId': buildingMaterialId}]
    })['attributeClassifications'][0]['classificationIds']
print ('Classified in the system: {}'.format (any (
    c['classificationSystemId']['guid'] == classificationSystemId['guid'] and c['classificationItemId']['guid'] == classificationItemId['guid']
    for c in classifications)))

# Omitting classificationItemId removes the classification in the given system.
aclib.RunTapirCommand (
    'SetClassificationsOfAttributes', {
        'attributeClassifications': [{
            'attributeId': buildingMaterialId,
            'classificationId': {
                'classificationSystemId': classificationSystemId
            }
        }]
    })

classifications = aclib.RunTapirCommand (
    'GetClassificationsOfAttributes', {
        'attributeIds': [{'attributeId': buildingMaterialId}]
    })['attributeClassifications'][0]['classificationIds']
print ('Classified in the system: {}'.format (any (
    c['classificationSystemId']['guid'] == classificationSystemId['guid']
    for c in classifications)))
