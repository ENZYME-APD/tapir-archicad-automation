import aclib

# Every field of UpdatePropertyDefinitions on a property this script creates and removes
# again, so it runs on any project. The point it checks: the property and its enum
# options keep their identifiers through rename, regroup and option edits, which is
# what keeps the values elements already hold.

GROUP_A = 'Tapir Example Update A'
GROUP_B = 'Tapir Example Update B'


def Find (name):
    for p in aclib.RunTapirCommand ('GetAllProperties', {}, debug = False)['properties']:
        if p['propertyType'] == 'Custom' and p['propertyName'] == name:
            return p
    return None


def Options (p):
    return {v['enumValue']['displayValue']: v['enumValue']['guid'] for v in p.get ('possibleEnumValues', [])}


groups = aclib.RunTapirCommand ('CreatePropertyGroups', {'propertyGroups': [
    {'propertyGroup': {'name': GROUP_A}}, {'propertyGroup': {'name': GROUP_B}}]}, debug = False)
groupB = groups['propertyGroupIds'][1]['propertyGroupId']

created = aclib.RunTapirCommand ('CreatePropertyDefinitions', {'propertyDefinitions': [{'propertyDefinition': {
    'name': 'Status', 'description': 'before', 'type': 'singleEnum', 'isEditable': True,
    'availability': [], 'group': {'name': GROUP_A},
    'possibleEnumValues': [{'enumValue': {'displayValue': v}} for v in ('Draft', 'Old', 'Approved')],
    'defaultValue': {'basicDefaultValue': {'status': 'normal', 'type': 'singleEnum',
                                           'value': {'type': 'displayValue', 'displayValue': 'Draft'}}}}}]}, debug = False)
propertyId = created['propertyIds'][0]['propertyId']
before = Options (Find ('Status'))

result = aclib.RunTapirCommand ('UpdatePropertyDefinitions', {'propertyDefinitions': [{
    'propertyId': propertyId,
    'name': 'Approval',
    'description': 'after',
    'groupId': groupB,
    'renameEnumValues': [{'enumValueId': {'guid': before['Draft']}, 'displayValue': 'In progress'}],
    'removeEnumValues': [{'enumValueId': {'guid': before['Old']}}],
    'possibleEnumValues': [{'enumValue': {'displayValue': 'Rejected'}}],
    'enumOrder': ['Approved', 'In progress', 'Rejected'],
    'defaultValue': {'basicDefaultValue': {'status': 'normal', 'type': 'singleEnum',
                                           'value': {'type': 'displayValue', 'displayValue': 'Approved'}}}}]}, debug = False)
print ('Update succeeded: {}'.format (result['executionResults'][0]['success']))

after = Find ('Approval')
print ('Same property guid: {}'.format (after['propertyId'] == propertyId))
print ('Group now: {}'.format (after['propertyGroupName']))
print ('Options now: {}'.format (list (Options (after))))
print ('Renamed option kept its id: {}'.format (Options (after)['In progress'] == before['Draft']))

refused = aclib.RunTapirCommand ('UpdatePropertyDefinitions', {'propertyDefinitions': [{
    'propertyId': propertyId,
    'removeEnumValues': [{'enumValueId': {'guid': before['Approved']}}]}]}, debug = False)
print ('Removing the default option alone is refused: {}'.format (not refused['executionResults'][0]['success']))

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [{'propertyId': propertyId}]}, debug = False)
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [
    groups['propertyGroupIds'][0], groups['propertyGroupIds'][1]]}, debug = False)
