"""Creates an editable custom text property with a default value with CreatePropertyDefinitions, then deletes it.

The property is put in a new group, created by name, and is available for no classification item.
"""
import aclib

group = 'Tapir Example Group'
propertyIds = aclib.RunTapirCommand ('CreatePropertyDefinitions', {
    'propertyDefinitions': [{
        'propertyDefinition': {
            'name': 'Tapir Example Text',
            'description': 'Created by a Tapir example',
            'type': 'string',
            'isEditable': True,
            'defaultValue': {'basicDefaultValue': {'type': 'string', 'status': 'normal', 'value': '-'}},
            'availability': [],
            'group': {'name': group}
        }
    }]
})['propertyIds']
aclib.PrintJson (propertyIds)

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': [p for p in propertyIds if 'propertyId' in p]})
groups = aclib.RunTapirCommand ('GetAllProperties', {'propertyTypes': ['Custom'], 'includeAvailability': False})['propertyGroups']
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': g['propertyGroupId']} for g in groups if g['name'] == group]})
