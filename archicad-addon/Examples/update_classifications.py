import aclib

# Creates a small classification system, renames the system and changes an item's code,
# then deletes it. The item keeps its identifier, which is what keeps elements
# classified with it and properties available for it.

SYSTEM = 'Tapir Example Update System'

aclib.RunTapirCommand ('CreateClassificationSystems', {'classificationSystemsWithItems': [{
    'classificationSystem': {'name': SYSTEM, 'description': '', 'source': '', 'version': '1', 'date': '2026-01-01'},
    'classificationItems': [{'id': '10', 'name': 'Walls', 'description': ''}]}]}, debug = False)


def FindSystem (name):
    for s in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']:
        if s['name'] == name:
            return s
    return None


system = FindSystem (SYSTEM)
items = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': system['classificationSystemId']})
item = items['classificationItems'][0]['classificationItem']

r1 = aclib.RunTapirCommand ('UpdateClassificationSystems', {'classificationSystems': [{
    'classificationSystemId': system['classificationSystemId'], 'name': SYSTEM + ' Renamed', 'version': '2', 'date': '2026-09-25'}]}, debug = False)
r2 = aclib.RunTapirCommand ('UpdateClassificationItems', {'classificationItems': [{
    'classificationItemId': item['classificationItemId'], 'id': '11', 'name': 'Walls, external'}]}, debug = False)
print ('System update succeeded: {}'.format (r1['executionResults'][0]['success']))
print ('Item update succeeded: {}'.format (r2['executionResults'][0]['success']))

renamed = FindSystem (SYSTEM + ' Renamed')
after = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': renamed['classificationSystemId']})
afterItem = after['classificationItems'][0]['classificationItem']
print ('Same system guid: {}'.format (renamed['classificationSystemId'] == system['classificationSystemId']))
print ('Item now: {} {}'.format (afterItem['id'], afterItem['name']))
print ('Same item guid: {}'.format (afterItem['classificationItemId'] == item['classificationItemId']))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [
    {'classificationSystemId': renamed['classificationSystemId']}]}, debug = False)
