"""Creates a Project Info group, moves it before the first group with MoveProjectInfoGroups, then deletes it.

Without beforeProjectInfoGroupId a group moves to the end of the list. Requires Archicad 30 or later.
"""
import aclib

firstGroupId = aclib.RunTapirCommand ('GetProjectInfoGroups')['projectInfoGroups'][0]['projectInfoGroupId']

created = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
    'projectInfoGroups': [{'name': 'Tapir Example Group'}]
})['projectInfoGroupIdsOrErrors'][0]
if 'error' in created:
    raise SystemExit ('The group could not be created.')
groupId = created['projectInfoGroupId']

aclib.PrintJson (aclib.RunTapirCommand ('MoveProjectInfoGroups', {
    'projectInfoGroups': [
        {'projectInfoGroupId': groupId, 'beforeProjectInfoGroupId': firstGroupId}
    ]
}))
aclib.RunTapirCommand ('GetProjectInfoGroups')

aclib.RunTapirCommand ('DeleteProjectInfoGroups', {'projectInfoGroups': [{'projectInfoGroupId': groupId}]})
