import sys
import aclib

# Manages the Project Info groups by script: lists the groups with their
# fields, creates a group, renames it, moves it to the front and then back to
# the end, and deletes it. The Project Info group commands need Archicad 30 or
# newer; on older versions the first command fails and the example stops.

groups = aclib.RunTapirCommand ('GetProjectInfoGroups')
if groups is None or 'error' in groups:
    print ('Could not get the Project Info groups, this example needs Archicad 30 or newer.')
    sys.exit ()

groupName = 'Tapir Example Group'
created = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
        'projectInfoGroups': [{'name': groupName}]
    })['projectInfoGroupIdsOrErrors'][0]
if 'error' in created:
    sys.exit ()
groupId = created['projectInfoGroupId']

aclib.RunTapirCommand ('RenameProjectInfoGroups', {
        'projectInfoGroups': [{'projectInfoGroupId': groupId, 'name': groupName + ' (renamed)'}]
    })

# Move: before the first group, then to the end (no beforeProjectInfoGroupId).
firstGroupId = groups['projectInfoGroups'][0]['projectInfoGroupId']
aclib.RunTapirCommand ('MoveProjectInfoGroups', {
        'projectInfoGroups': [{'projectInfoGroupId': groupId, 'beforeProjectInfoGroupId': firstGroupId}]
    })
aclib.RunTapirCommand ('GetProjectInfoGroups')
aclib.RunTapirCommand ('MoveProjectInfoGroups', {
        'projectInfoGroups': [{'projectInfoGroupId': groupId}]
    })

# Delete: only a group without fixed (built-in) fields can be deleted.
aclib.RunTapirCommand ('DeleteProjectInfoGroups', {
        'projectInfoGroups': [{'projectInfoGroupId': groupId}]
    })
aclib.RunTapirCommand ('GetProjectInfoGroups')
