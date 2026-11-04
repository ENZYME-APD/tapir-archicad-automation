"""Creates a Project Info group and deletes it with DeleteProjectInfoGroups.

A group that holds built-in fields cannot be deleted. Requires Archicad 30 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoGroups', {
    'projectInfoGroups': [{'name': 'Tapir Group To Delete'}]
})['projectInfoGroupIdsOrErrors'][0]
if 'error' in created:
    raise SystemExit ('The group could not be created.')

aclib.PrintJson (aclib.RunTapirCommand ('DeleteProjectInfoGroups', {
    'projectInfoGroups': [
        {'projectInfoGroupId': created['projectInfoGroupId']}
    ]
}))
