"""Saves a project data, reserves it in Teamwork with ReserveProjectData, then releases and deletes it.

Needs a Teamwork project; in a solo project the reservation fails.
"""
import aclib

name = 'tapir-example/teamwork'
aclib.RunTapirCommand ('SetProjectData', {'values': [{'name': name, 'content': 'shared'}]})

aclib.PrintJson (aclib.RunTapirCommand ('ReserveProjectData', {'names': [name]}))
aclib.RunTapirCommand ('GetProjectDataKeys')

aclib.RunTapirCommand ('ReleaseProjectData', {'names': [name]})
aclib.RunTapirCommand ('DeleteProjectData', {'names': [name]})
