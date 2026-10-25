import aclib

# Issue #944: the classification of a tool's defaults (the settings new elements get) is
# read and set directly with GetClassificationsOfElementDefaults and
# SetClassificationsOfElementDefaults. Sets a Wall default, creates a wall, reads its
# classification, then deletes the wall and restores the default.

SYSTEM = 'Tapir Example Defaults System'

aclib.RunTapirCommand ('CreateClassificationSystems', {'classificationSystemsWithItems': [{
    'classificationSystem': {'name': SYSTEM, 'description': '', 'source': '', 'version': '1', 'date': '2026-01-01'},
    'classificationItems': [{'id': '10', 'name': 'Walls', 'description': ''}]}]}, debug = False)

system = [s for s in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems'] if s['name'] == SYSTEM][0]
systemId = system['classificationSystemId']
item = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']

def DefaultItem ():
    response = aclib.RunTapirCommand ('GetClassificationsOfElementDefaults', {
        'elementTypes': ['Wall'],
        'classificationSystemIds': [{'classificationSystemId': systemId}]}, debug = False)
    classification = response['elementDefaultClassifications'][0]['classificationIds'][0]
    return classification.get ('classificationItemId')

previousDefault = DefaultItem ()

result = aclib.RunTapirCommand ('SetClassificationsOfElementDefaults', {'elementDefaultClassifications': [{
    'elementType': 'Wall',
    'classificationId': {'classificationSystemId': systemId, 'classificationItemId': item['classificationItemId']}}]}, debug = False)
print ('Setting the Wall default succeeded: {}'.format (result['executionResults'][0]['success']))
print ('Wall default is the item: {}'.format (DefaultItem () == item['classificationItemId']))

wall = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 6000.0, 'y': 0.0},
    'endCoordinate': {'x': 6005.0, 'y': 0.0}}]}, debug = False)['elements'][0]
if 'elementId' in wall:
    response = aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements': [{'elementId': wall['elementId']}],
        'classificationSystemIds': [{'classificationSystemId': systemId}]}, debug = False)
    wallItem = response['elementClassifications'][0]['classificationIds'][0].get ('classificationItemId')
    print ('New wall has the default classification: {}'.format (wallItem == item['classificationItemId']))
    aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': wall['elementId']}]}, debug = False)

# Unclassified again (no classificationItemId), then the previous default, if it had one.
restore = {'classificationSystemId': systemId}
if previousDefault is not None:
    restore['classificationItemId'] = previousDefault
result = aclib.RunTapirCommand ('SetClassificationsOfElementDefaults', {'elementDefaultClassifications': [{
    'elementType': 'Wall', 'classificationId': restore}]}, debug = False)
print ('Restoring the Wall default succeeded: {}'.format (result['executionResults'][0]['success']))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [
    {'classificationSystemId': systemId}]}, debug = False)
