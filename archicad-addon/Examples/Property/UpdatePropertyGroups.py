"""Creates a temporary custom property group and changes its name and description with UpdatePropertyGroups, then deletes it."""
import aclib

groupId = aclib.RunTapirCommand ('CreatePropertyGroups', {
    'propertyGroups': [{'propertyGroup': {'name': 'Tapir Example Group'}}]
})['propertyGroupIds'][0]
if 'error' in groupId:
    raise SystemExit ('Could not create the property group: ' + groupId['error']['message'])

aclib.PrintJson (aclib.RunTapirCommand ('UpdatePropertyGroups', {
    'propertyGroups': [{
        'propertyGroupId': groupId['propertyGroupId'],
        'name': 'Tapir Example Group Renamed',
        'description': 'Updated by a Tapir example'
    }]
}))

aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [groupId]})
