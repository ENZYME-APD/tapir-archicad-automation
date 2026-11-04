"""Creates a custom Project Info field and deletes it by its autotext key with DeleteProjectInfoFields.

Built-in fields cannot be deleted. From Archicad 30 fields can also be deleted by projectInfoFieldIds.
"""
import aclib

created = aclib.RunTapirCommand ('CreateProjectInfoFields', {
    'projectInfoFields': [{'projectInfoName': 'Tapir Field To Delete'}]
})['fields'][0]
if 'projectInfoId' not in created:
    raise SystemExit ('The field could not be created.')

aclib.PrintJson (aclib.RunTapirCommand ('DeleteProjectInfoFields', {'projectInfoIds': [created['projectInfoId']]}))
