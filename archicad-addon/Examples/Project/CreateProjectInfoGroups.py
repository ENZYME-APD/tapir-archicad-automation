"""Creates a Project Info group at the end of the group list with CreateProjectInfoGroups, then deletes it.

Requires Archicad 30 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
    'projectInfoGroups': [{'name': 'Tapir Example Group'}]
})['projectInfoGroupIdsOrErrors'][0]
aclib.PrintJson (created)
if 'error' in created:
    raise SystemExit ('The group could not be created.')

aclib.RunTapirCommand ('GetProjectInfoGroups')

aclib.RunTapirCommand ('DeleteProjectInfoGroups', {'projectInfoGroups': [{'projectInfoGroupId': created['projectInfoGroupId']}]})
