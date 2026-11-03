import aclib

# Scripts can keep their own state on attributes and groups, invisible in the Archicad UI: string
# values under keys, stored in the user data of the attribute or the group and saved with the
# project. Setting merges the given keys into what is already stored, so scripts with different
# keys do not disturb each other. Prefix the keys with the script's name.
# Archicad does not record changes of attribute user data as undo steps: Undo does not revert them.

layers = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'})['attributes'][:2]
attributeIds = [{'attributeId': layer['attributeId']} for layer in layers]

aclib.RunTapirCommand ('SetUserDataOfAttributes', {
    'attributeIds': attributeIds,
    'pairs': [
        {'key': 'example.sync.state', 'value': 'new'},
        {'key': 'example.owner', 'value': 'tapir'}
    ]
})

# Setting a key again replaces its value; keysToDelete removes keys.
aclib.RunTapirCommand ('SetUserDataOfAttributes', {
    'attributeIds': attributeIds,
    'pairs': [
        {'key': 'example.sync.state', 'value': 'synced'}
    ],
    'keysToDelete': ['example.owner']
})

aclib.RunTapirCommand ('GetUserDataOfAttributes', {'attributeIds': attributeIds})

aclib.RunTapirCommand ('DeleteUserDataOfAttributes', {'attributeIds': attributeIds})

aclib.RunTapirCommand ('GetUserDataOfAttributes', {'attributeIds': attributeIds})

# Groups: the pairs are stored in one undo step, and GetUserDataOfGroups returns them as pairs
# next to the raw bytes.
beams = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Beam'})['elements']
groupsOfBeams = aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': beams})['groupGuids']

groupIds = []
for groupIdOrError in groupsOfBeams:
    if 'groupId' in groupIdOrError and groupIdOrError not in groupIds:
        groupIds.append (groupIdOrError)

if len (groupIds) == 0:
    print ('No beam of the project is part of a group.')
else:
    aclib.RunTapirCommand ('SetUserDataOfGroups', {
        'groups': groupIds,
        'pairs': [
            {'key': 'example.group.role', 'value': 'beam group'}
        ]
    })

    response = aclib.RunTapirCommand ('GetUserDataOfGroups', {'groups': groupIds})
    for groupId, userDataOrError in zip (groupIds, response['userDataOfGroups']):
        if 'error' in userDataOrError:
            print (groupId['groupId']['guid'], userDataOrError['error']['message'])
        else:
            print (groupId['groupId']['guid'], userDataOrError['userData'].get ('pairs'))

    aclib.RunTapirCommand ('SetUserDataOfGroups', {
        'groups': groupIds,
        'keysToDelete': ['example.group.role']
    })
