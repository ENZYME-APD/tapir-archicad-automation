"""Creates a temporary custom property and changes its name, description and default value with UpdatePropertyDefinitions.

The property keeps its guid. The script deletes the property and its group again at the end.
"""
import aclib

groupId = aclib.RunTapirCommand ('CreatePropertyGroups', {
    'propertyGroups': [{'propertyGroup': {'name': 'Tapir Example Group'}}]
})['propertyGroupIds'][0]
if 'error' in groupId:
    raise SystemExit ('Could not create the property group: ' + groupId['error']['message'])
propertyId = aclib.RunTapirCommand ('CreatePropertyDefinitions', {
    'propertyDefinitions': [{'propertyDefinition': {
        'name': 'Tapir Example Property', 'description': '', 'type': 'string', 'isEditable': True,
        'availability': [], 'group': {'propertyGroupId': groupId['propertyGroupId']}
    }}]
})['propertyIds'][0]

aclib.PrintJson (aclib.RunTapirCommand ('UpdatePropertyDefinitions', {
    'propertyDefinitions': [{
        'propertyId': propertyId['propertyId'],
        'name': 'Tapir Example Property Renamed',
        'description': 'Updated by a Tapir example',
        'defaultValue': {'basicDefaultValue': {'type': 'string', 'status': 'normal', 'value': 'n/a'}}
    }]
}))
aclib.RunTapirCommand ('GetAllProperties', {'properties': [propertyId]})

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [propertyId]})
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [groupId]})
