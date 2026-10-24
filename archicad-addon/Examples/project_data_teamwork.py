import aclib

# Shared project data is reserved and released in Teamwork like other Teamwork data. In a solo
# project these commands report the error Archicad returns.
name = 'tapir-example/teamwork'

aclib.RunTapirCommand ('SetProjectData', {
    'values': [
        {'name': name, 'content': 'shared by every Teamwork user'}
    ]
})

aclib.RunTapirCommand ('ReserveProjectData', {
    'names': [name]
})

# The owner is listed while the project data is reserved.
aclib.RunTapirCommand ('GetProjectDataKeys')

aclib.RunTapirCommand ('ReleaseProjectData', {
    'names': [name]
})

aclib.RunTapirCommand ('RequestProjectData', {
    'names': [name],
    'mailText': 'Please release the Tapir example data.'
})

# Grant the project data to a user: reserve it first, then pass the userId of a user, for
# example one from the conflicts of ReserveProjectData.
# aclib.RunTapirCommand ('GrantProjectData', {
#     'names': [name],
#     'toUserId': 2
# })

aclib.RunTapirCommand ('DeleteProjectData', {
    'names': [name]
})
