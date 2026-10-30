import aclib

# ConvertUnitValues formats values given in meters, square meters, cubic meters
# or radians as Archicad shows them with the unit of a Project Preference.
# Available from Archicad 27.
convertedValues = aclib.RunTapirCommand ('ConvertUnitValues', {
    'values': [
        {'value': 2.75, 'preference': 'WorkModel'},
        {'value': 12.5, 'preference': 'CalcArea'},
        {'value': 3.0, 'preference': 'CalcVolume'},
        {'value': 0.785398, 'preference': 'WorkAngle'}
    ]
}, debug = False)
if convertedValues and 'convertedValues' in convertedValues:
    for convertedValue in convertedValues['convertedValues']:
        if 'error' in convertedValue:
            print ('Error: {}'.format (convertedValue['error']['message']))
        else:
            print ('{}{} {}'.format (convertedValue['convertedValue'], convertedValue['extraAccuracy'], convertedValue['unit']))

printerAndPlotterInfo = aclib.RunTapirCommand ('GetPrinterAndPlotterInfo')

# The localized names of the renovation statuses. Available from Archicad 27.
renovationStatusNames = aclib.RunTapirCommand ('GetRenovationStatusNames')

# GetClassificationItemParents returns the system and the parent of each item;
# a top level item has no parent.
SYSTEM = 'Tapir Example Parents System'

aclib.RunTapirCommand ('CreateClassificationSystems', {'classificationSystemsWithItems': [{
    'classificationSystem': {'name': SYSTEM, 'description': '', 'source': '', 'version': '1', 'date': '2026-01-01'},
    'classificationItems': [{'id': '10', 'name': 'Walls', 'description': '', 'children': [
        {'id': '10.1', 'name': 'External walls', 'description': ''}]}]}]}, debug = False)

system = next (s for s in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems'] if s['name'] == SYSTEM)
topItem = aclib.RunCommand ('API.GetAllClassificationsInSystem', {
    'classificationSystemId': system['classificationSystemId']})['classificationItems'][0]['classificationItem']
childItem = topItem['children'][0]['classificationItem']

itemParents = aclib.RunTapirCommand ('GetClassificationItemParents', {'classificationItemIds': [
    {'classificationItemId': topItem['classificationItemId']},
    {'classificationItemId': childItem['classificationItemId']}]}, debug = False)['classificationItemParents']

for item, itemParent in zip ([topItem, childItem], itemParents):
    if 'error' in itemParent:
        print ('{}: {}'.format (item['id'], itemParent['error']['message']))
        continue
    print ('{} is in the system: {}'.format (item['id'], itemParent['classificationSystemId'] == system['classificationSystemId']))
    if 'parentClassificationItemId' in itemParent:
        print ('{} has the parent {}: {}'.format (item['id'], topItem['id'], itemParent['parentClassificationItemId'] == topItem['classificationItemId']))
    else:
        print ('{} is a top level item'.format (item['id']))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [
    {'classificationSystemId': system['classificationSystemId']}]}, debug = False)
