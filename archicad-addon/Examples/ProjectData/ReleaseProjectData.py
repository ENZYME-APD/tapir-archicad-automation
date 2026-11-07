"""Saves and reserves a project data, then releases it in Teamwork with ReleaseProjectData and deletes it.

Needs a Teamwork project; in a solo project the reservation and the release fail.
"""
import aclib

name = 'tapir-example/teamwork'
aclib.RunTapirCommand ('SetProjectData', {'values': [{'name': name, 'content': 'shared'}]})
aclib.RunTapirCommand ('ReserveProjectData', {'names': [name]})

aclib.PrintJson (aclib.RunTapirCommand ('ReleaseProjectData', {'names': [name]}))
aclib.RunTapirCommand ('GetProjectDataKeys')

aclib.RunTapirCommand ('DeleteProjectData', {'names': [name]})
