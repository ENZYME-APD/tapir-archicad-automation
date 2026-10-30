import aclib

# Checks which items of the first classification system are visible (offered)
# for the first wall, the Wall tool default and the first building material.

def CollectItems (items, result):
    for item in items:
        classificationItem = item['classificationItem']
        result.append (classificationItem)
        CollectItems (classificationItem.get ('children', []), result)

system = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems'][0]
tree = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': system['classificationSystemId']})
items = []
CollectItems (tree['classificationItems'], items)

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']
buildingMaterials = aclib.RunCommand ('API.GetAttributesByType', {'attributeType': 'BuildingMaterial'})['attributeIds']

targets = [{'elementType': 'Wall'}]
if walls:
    targets.append ({'elementId': walls[0]['elementId']})
if buildingMaterials:
    targets.append ({'attributeId': buildingMaterials[0]['attributeId']})

response = aclib.RunTapirCommand ('GetClassificationItemVisibility', {
    'targets': targets,
    'classificationItemIds': [{'classificationItemId': item['classificationItemId']} for item in items]
}, debug = False)

for target, result in zip (targets, response['classificationItemVisibilities']):
    targetName = list (target.keys ())[0]
    if 'error' in result:
        print ('{}: {}'.format (targetName, result['error']['message']))
        continue
    visibleItems = [item['id'] for item, visibility in zip (items, result['visibilities']) if visibility.get ('isVisible')]
    print ('{}: {} of {} items visible: {}'.format (targetName, len (visibleItems), len (items), ', '.join (visibleItems)))
