"""Creates a custom Project Info field, renames it with RenameProjectInfoFields, then deletes it.

Built-in fields cannot be renamed. Requires Archicad 30 or later.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoFields', {
    'projectInfoFields': [{'projectInfoName': 'Tapir Example Field'}]
})['fields'][0]
if 'projectInfoFieldId' not in created:
    raise SystemExit ('The field could not be created.')
fieldId = created['projectInfoFieldId']

aclib.PrintJson (aclib.RunTapirCommand ('RenameProjectInfoFields', {
    'projectInfoFields': [
        {'projectInfoFieldId': fieldId, 'name': 'Tapir Renamed Field'}
    ]
}))
aclib.RunTapirCommand ('GetProjectInfoGroups')

aclib.RunTapirCommand ('DeleteProjectInfoFields', {'projectInfoFieldIds': [{'projectInfoFieldId': fieldId}]})
