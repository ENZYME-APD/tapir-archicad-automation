"""Creates a custom Project Info field with CreateProjectInfoFields, then deletes it again.

From Archicad 30 the new field goes to the first Project Info group unless projectInfoGroupId is given.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoFields', {
    'projectInfoFields': [
        {'projectInfoName': 'Tapir Example Field', 'projectInfoValue': 'Created by Tapir'}
    ]
})['fields'][0]
aclib.PrintJson (created)
if 'error' in created:
    raise SystemExit ('The field could not be created.')

aclib.RunTapirCommand ('GetProjectInfoFields')

if 'projectInfoFieldId' in created:
    aclib.RunTapirCommand ('DeleteProjectInfoFields', {'projectInfoFieldIds': [{'projectInfoFieldId': created['projectInfoFieldId']}]})
else:
    aclib.RunTapirCommand ('DeleteProjectInfoFields', {'projectInfoIds': [created['projectInfoId']]})
