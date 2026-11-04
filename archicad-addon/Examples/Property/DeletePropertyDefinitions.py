"""Creates a temporary custom property and deletes it again with DeletePropertyDefinitions."""
import aclib

groupId = aclib.RunTapirCommand ('CreatePropertyGroups', {
    'propertyGroups': [{'propertyGroup': {'name': 'Tapir Example Group'}}]
})['propertyGroupIds'][0]
if 'error' in groupId:
    raise SystemExit ('Could not create the property group: ' + groupId['error']['message'])
propertyIds = aclib.RunTapirCommand ('CreatePropertyDefinitions', {
    'propertyDefinitions': [{'propertyDefinition': {
        'name': 'Tapir Example Property To Delete', 'description': '', 'type': 'string', 'isEditable': True,
        'availability': [], 'group': {'propertyGroupId': groupId['propertyGroupId']}
    }}]
})['propertyIds']

aclib.PrintJson (aclib.RunTapirCommand ('DeletePropertyDefinitions', {
    'propertyIds': [p for p in propertyIds if 'propertyId' in p]
}))

aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [groupId]})
