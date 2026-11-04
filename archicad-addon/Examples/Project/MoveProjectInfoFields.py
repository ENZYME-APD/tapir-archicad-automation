"""Creates a custom Project Info field, moves it to the top of the first group with MoveProjectInfoFields, then deletes it.

Requires Archicad 30 or later.
"""
import aclib

firstGroup = aclib.RunTapirCommand ('GetProjectInfoGroups')['projectInfoGroups'][0]

created = aclib.RunTapirCommand ('CreateProjectInfoFields', {
    'projectInfoFields': [{'projectInfoName': 'Tapir Example Field', 'projectInfoGroupId': firstGroup['projectInfoGroupId']}]
})['fields'][0]
if 'projectInfoFieldId' not in created:
    raise SystemExit ('The field could not be created.')
fieldId = created['projectInfoFieldId']

move = {'projectInfoFieldId': fieldId, 'projectInfoGroupId': firstGroup['projectInfoGroupId']}
if firstGroup['fields']:
    move['beforeProjectInfoFieldId'] = firstGroup['fields'][0]['projectInfoFieldId']
aclib.PrintJson (aclib.RunTapirCommand ('MoveProjectInfoFields', {'projectInfoFields': [move]}))
aclib.RunTapirCommand ('GetProjectInfoGroups')

aclib.RunTapirCommand ('DeleteProjectInfoFields', {'projectInfoFieldIds': [{'projectInfoFieldId': fieldId}]})
