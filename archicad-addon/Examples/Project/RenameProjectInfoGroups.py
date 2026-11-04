"""Creates a Project Info group, renames it with RenameProjectInfoGroups, then deletes it.

Requires Archicad 30 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
    'projectInfoGroups': [{'name': 'Tapir Example Group'}]
})['projectInfoGroupIdsOrErrors'][0]
if 'error' in created:
    raise SystemExit ('The group could not be created.')
groupId = created['projectInfoGroupId']

aclib.PrintJson (aclib.RunTapirCommand ('RenameProjectInfoGroups', {
    'projectInfoGroups': [
        {'projectInfoGroupId': groupId, 'name': 'Tapir Renamed Group'}
    ]
}))
aclib.RunTapirCommand ('GetProjectInfoGroups')

aclib.RunTapirCommand ('DeleteProjectInfoGroups', {'projectInfoGroups': [{'projectInfoGroupId': groupId}]})
