"""Creates an issue and attaches the walls of the project to it as highlighted elements with AttachElementsToIssue.

Needs at least one Wall. Deletes the issue at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue'})

aclib.PrintJson (aclib.RunTapirCommand ('AttachElementsToIssue', {'issueId': issue['issueId'], 'elements': walls, 'type': 'Highlight'}))
aclib.RunTapirCommand ('GetElementsAttachedToIssue', {'issueId': issue['issueId'], 'type': 'Highlight'})

aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']})
