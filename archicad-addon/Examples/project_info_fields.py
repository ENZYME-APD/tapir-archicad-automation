import sys
import aclib

# Manages the custom Project Info fields by script: creates a field, sets its
# value through its autotext key and deletes it. On Archicad 30 or newer it
# also creates a group with a field in it, renames the field, moves it to the
# first group and back, and deletes the field and the group; on older versions
# the group part is skipped.

fieldName = 'Tapir Example Field'
response = aclib.RunTapirCommand ('CreateProjectInfoFields', {
        'projectInfoFields': [{'projectInfoName': fieldName, 'projectInfoValue': 'Created by Tapir'}]
    })
if response is None or 'error' in response or 'error' in response['fields'][0]:
    sys.exit ()
created = response['fields'][0]

# The autotext key is the id that SetProjectInfoField takes. On Archicad 30 the
# key is given only when the name of the new field is unique among the fields.
if 'projectInfoId' in created:
    aclib.RunTapirCommand ('SetProjectInfoField', {
            'projectInfoId': created['projectInfoId'],
            'projectInfoValue': 'Changed by Tapir'
        })
    aclib.RunTapirCommand ('DeleteProjectInfoFields', {
            'projectInfoIds': [created['projectInfoId']]
        })
elif 'projectInfoFieldId' in created:
    aclib.RunTapirCommand ('DeleteProjectInfoFields', {
            'projectInfoFieldIds': [{'projectInfoFieldId': created['projectInfoFieldId']}]
        })

# The rest needs the Project Info groups of Archicad 30.
groups = aclib.RunTapirCommand ('GetProjectInfoGroups')
if groups is None or 'error' in groups:
    print ('The Project Info group part of this example needs Archicad 30 or newer.')
    sys.exit ()

createdGroup = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
        'projectInfoGroups': [{'name': 'Tapir Example Group'}]
    })['projectInfoGroupIdsOrErrors'][0]
if 'error' in createdGroup:
    sys.exit ()
groupId = createdGroup['projectInfoGroupId']

created = aclib.RunTapirCommand ('CreateProjectInfoFields', {
        'projectInfoFields': [{'projectInfoName': fieldName, 'projectInfoValue': 'In a group', 'projectInfoGroupId': groupId}]
    })['fields'][0]
if 'error' in created:
    sys.exit ()
fieldId = created['projectInfoFieldId']

aclib.RunTapirCommand ('RenameProjectInfoFields', {
        'projectInfoFields': [{'projectInfoFieldId': fieldId, 'name': fieldName + ' (renamed)'}]
    })

# Move: to the front of the first group, then back to the end of the example's group.
firstGroup = groups['projectInfoGroups'][0]
move = {'projectInfoFieldId': fieldId, 'projectInfoGroupId': firstGroup['projectInfoGroupId']}
if firstGroup['fields']:
    move['beforeProjectInfoFieldId'] = firstGroup['fields'][0]['projectInfoFieldId']
aclib.RunTapirCommand ('MoveProjectInfoFields', {'projectInfoFields': [move]})
aclib.RunTapirCommand ('GetProjectInfoGroups')
aclib.RunTapirCommand ('MoveProjectInfoFields', {
        'projectInfoFields': [{'projectInfoFieldId': fieldId, 'projectInfoGroupId': groupId}]
    })

# Delete: the field by its id, then the group, which is empty again.
aclib.RunTapirCommand ('DeleteProjectInfoFields', {
        'projectInfoFieldIds': [{'projectInfoFieldId': fieldId}]
    })
aclib.RunTapirCommand ('DeleteProjectInfoGroups', {
        'projectInfoGroups': [{'projectInfoGroupId': groupId}]
    })
aclib.RunTapirCommand ('GetProjectInfoGroups')
