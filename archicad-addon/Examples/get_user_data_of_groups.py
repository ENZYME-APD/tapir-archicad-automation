import base64
import aclib

# Reads the user data add-ons attached to the groups of the beams of the project.
# Groups made in Archicad itself carry no user data, so most groups answer with an error.

beams = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Beam'})['elements']
groupsOfBeams = aclib.RunTapirCommand ('GetGroupsOfElements', {'elements': beams})['groupGuids']

groupIds = []
for groupIdOrError in groupsOfBeams:
    if 'groupId' in groupIdOrError and groupIdOrError not in groupIds:
        groupIds.append (groupIdOrError)

if len (groupIds) == 0:
    print ('No beam of the project is part of a group.')
else:
    response = aclib.RunTapirCommand ('GetUserDataOfGroups', {'groups': groupIds})
    for groupId, userDataOrError in zip (groupIds, response['userDataOfGroups']):
        if 'error' in userDataOrError:
            print (groupId['groupId']['guid'], userDataOrError['error']['message'])
        else:
            userData = userDataOrError['userData']
            data = base64.b64decode (userData['data'])
            print (groupId['groupId']['guid'], 'version', userData['dataVersion'], 'flags', userData['flags'], len (data), 'bytes')
