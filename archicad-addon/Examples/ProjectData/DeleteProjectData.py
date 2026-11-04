"""Saves a project data and deletes it again with DeleteProjectData."""
import aclib

name = 'tapir-example/to-delete'
aclib.RunTapirCommand ('SetProjectData', {'values': [{'name': name, 'content': 'temporary'}]})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteProjectData', {'names': [name]}))
