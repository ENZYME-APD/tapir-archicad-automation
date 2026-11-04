"""Saves and reserves a project data, then grants it to another Teamwork user with GrantProjectData.

Needs a Teamwork project. Set toUserId to the user to grant to, e.g. the userId of a teamworkOwner
that GetProjectDataKeys lists.
"""
import aclib

toUserId = 2
name = 'tapir-example/teamwork'

aclib.RunTapirCommand ('SetProjectData', {'values': [{'name': name, 'content': 'shared'}]})
aclib.RunTapirCommand ('ReserveProjectData', {'names': [name]})

aclib.PrintJson (aclib.RunTapirCommand ('GrantProjectData', {'names': [name], 'toUserId': toUserId}))
