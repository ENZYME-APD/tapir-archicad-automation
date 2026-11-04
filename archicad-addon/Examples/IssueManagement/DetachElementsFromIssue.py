"""Creates an issue, attaches the walls of the project to it, then detaches them with DetachElementsFromIssue.

Needs at least one Wall. Deletes the issue at the end.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue'})
aclib.RunTapirCommand ('AttachElementsToIssue', {'issueId': issue['issueId'], 'elements': walls, 'type': 'Highlight'})

aclib.PrintJson (aclib.RunTapirCommand ('DetachElementsFromIssue', {'issueId': issue['issueId'], 'elements': walls}))
aclib.RunTapirCommand ('GetElementsAttachedToIssue', {'issueId': issue['issueId'], 'type': 'Highlight'})

aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']})
