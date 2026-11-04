"""Requests the project data reserved by other Teamwork users from them, with a message, with RequestProjectData.

Needs a Teamwork project. Sends the request to the users who reserved the data.
"""
import aclib

keys = aclib.RunTapirCommand ('GetProjectDataKeys')['keys']
reserved = [k['name'] for k in keys if 'teamworkOwner' in k]
if not reserved:
    raise SystemExit ('No reserved project data.')

aclib.PrintJson (aclib.RunTapirCommand ('RequestProjectData', {
    'names': reserved,
    'mailText': 'Please release the Tapir project data.'
}))
