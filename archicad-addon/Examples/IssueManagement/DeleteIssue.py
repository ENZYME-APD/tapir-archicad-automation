"""Creates an issue and deletes it with DeleteIssue."""
import aclib

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue'})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']}))
