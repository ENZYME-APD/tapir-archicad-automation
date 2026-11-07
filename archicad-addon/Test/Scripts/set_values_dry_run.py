"""SetPropertyValuesOfElements and SetClassificationsOfElements with dryRun (#2195): previews a new
Element ID and a new classification for the first wall, checks that the project is unchanged
afterwards and that wouldChange gives the old and new values, then makes the changes for real and
restores them.

Prints only values and results, no guids, so the output can be recorded as a golden."""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug=False)['elements'][:1]
assert walls, 'No wall in the project.'
wallId = walls[0]['elementId']
missing = {'guid': '00000000-0000-0000-0000-000000000001'}

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]
})['properties'][0]['propertyId']
systemId = aclib.RunTapirCommand ('GetAllClassificationSystems', {}, debug=False)['classificationSystems'][0]['classificationSystemId']
itemId = aclib.RunCommand ('API.GetAllClassificationsInSystem', {'classificationSystemId': systemId})['classificationItems'][0]['classificationItem']['classificationItemId']


def ReadElementId ():
    return aclib.RunTapirCommand ('GetPropertyValuesOfElements', {
        'elements': walls,
        'properties': [{'propertyId': elementIdPropertyId}]
    }, debug=False)['propertyValuesForElements'][0]['propertyValues'][0]['propertyValue']['value']


def ReadClassification ():
    return aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements': walls,
        'classificationSystemIds': [{'classificationSystemId': systemId}]
    }, debug=False)['elementClassifications'][0]['classificationIds'][0]


oldElementId = ReadElementId ()
oldClassification = ReadClassification ()

# Properties: one value that would be written, one for an element that does not exist.
propertyItems = [
    {'elementId': wallId, 'propertyId': elementIdPropertyId, 'propertyValue': {'value': 'TAPIR-DRY-RUN'}},
    {'elementId': missing, 'propertyId': elementIdPropertyId, 'propertyValue': {'value': 'TAPIR-DRY-RUN'}},
]
preview = aclib.RunTapirCommand ('SetPropertyValuesOfElements', {'elementPropertyValues': propertyItems, 'dryRun': True}, debug=False)
wouldChange = preview['executionResults'][0].get ('wouldChange', {})
print ('property: dry run flag in the response:', preview.get ('dryRun'))
print ('property: results:', len (preview['executionResults']))
print ('property: wouldChange old is the value now:', wouldChange.get ('oldValue') == oldElementId)
print ('property: wouldChange new:', wouldChange.get ('newValue'))
print ('property: missing element fails:', preview['executionResults'][1]['success'] is False)
print ('property: unchanged by the dry run:', ReadElementId () == oldElementId)
assert preview.get ('dryRun') is True
assert ReadElementId () == oldElementId

# Classifications: classify the wall with the first item, and an element that does not exist.
classificationItems = [
    {'elementId': wallId, 'classificationId': {'classificationSystemId': systemId, 'classificationItemId': itemId}},
    {'elementId': missing, 'classificationId': {'classificationSystemId': systemId, 'classificationItemId': itemId}},
]
preview = aclib.RunTapirCommand ('SetClassificationsOfElements', {'elementClassifications': classificationItems, 'dryRun': True}, debug=False)
wouldChange = preview['executionResults'][0].get ('wouldChange', {})
print ('classification: dry run flag in the response:', preview.get ('dryRun'))
print ('classification: wouldChange old is the classification now:', wouldChange.get ('oldValue') == oldClassification)
print ('classification: wouldChange new is the given item:', wouldChange.get ('newValue', {}).get ('classificationItemId') == itemId)
print ('classification: missing element fails:', preview['executionResults'][1]['success'] is False)
print ('classification: unchanged by the dry run:', ReadClassification () == oldClassification)
assert preview.get ('dryRun') is True
assert ReadClassification () == oldClassification

# The real runs change the values and give no dry run fields; then the old values are restored.
result = aclib.RunTapirCommand ('SetPropertyValuesOfElements', {'elementPropertyValues': propertyItems[:1]}, debug=False)
print ('property: real run has no dry run fields:', 'dryRun' not in result and all ('wouldChange' not in r for r in result['executionResults']))
print ('property: changed by the real run:', ReadElementId ())
result = aclib.RunTapirCommand ('SetClassificationsOfElements', {'elementClassifications': classificationItems[:1]}, debug=False)
print ('classification: real run has no dry run fields:', 'dryRun' not in result and all ('wouldChange' not in r for r in result['executionResults']))
print ('classification: changed by the real run:', ReadClassification ().get ('classificationItemId') == itemId)

aclib.RunTapirCommand ('SetPropertyValuesOfElements', {'elementPropertyValues': [
    {'elementId': wallId, 'propertyId': elementIdPropertyId, 'propertyValue': {'value': oldElementId}}]}, debug=False)
aclib.RunTapirCommand ('SetClassificationsOfElements', {'elementClassifications': [
    {'elementId': wallId, 'classificationId': oldClassification}]}, debug=False)
print ('restored:', ReadElementId () == oldElementId and ReadClassification () == oldClassification)
