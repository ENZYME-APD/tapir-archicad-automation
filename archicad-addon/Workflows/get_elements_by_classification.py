import aclib

# Lists how many elements are classified with each item of every classification system,
# with one GetElementsByClassification call per system.


def CollectItems (classificationItems, items):
    for classificationItem in classificationItems:
        item = classificationItem['classificationItem']
        items.append (item)
        CollectItems (item.get ('children', []), items)


for system in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']:
    tree = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': system['classificationSystemId']})
    items = []
    CollectItems (tree['classificationItems'], items)
    if not items:
        continue

    response = aclib.RunTapirCommand ('GetElementsByClassification', {
            'classificationItemIds': [{'classificationItemId': item['classificationItemId']} for item in items]
        }, debug = False)

    print ('{} {}'.format (system['name'], system['version']))
    for item, result in zip (items, response['elementsOfClassificationItems']):
        if 'error' in result:
            print ('    {} {}: {}'.format (item['id'], item['name'], result['error']['message']))
        elif result['elements']:
            print ('    {} {}: {} elements'.format (item['id'], item['name'], len (result['elements'])))
