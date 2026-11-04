"""Sets the client and the project status of the Project Notes with SetProjectNotes, reads them back, then restores them.

Only the given fields change.
"""
import aclib

original = aclib.RunTapirCommand ('GetProjectNotes')

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectNotes', {
    'client': 'Tapir Client Ltd.',
    'projectStatus': 'Design development'
}))
aclib.RunTapirCommand ('GetProjectNotes')

aclib.PrintJson (aclib.RunTapirCommand ('SetProjectNotes', {
    'client': original['client'],
    'projectStatus': original['projectStatus']
}))
