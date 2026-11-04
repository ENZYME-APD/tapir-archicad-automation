"""Prints the content of every project data stored by Tapir with GetProjectData."""
import aclib

keys = aclib.RunTapirCommand ('GetProjectDataKeys')['keys']
if not keys:
    raise SystemExit ('No project data in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetProjectData', {'names': [k['name'] for k in keys]}))
