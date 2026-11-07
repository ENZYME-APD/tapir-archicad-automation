"""Creates an issue with a tag text using CreateIssue, then deletes it again."""
import aclib

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue', 'tagText': 'Tapir'})
aclib.PrintJson (issue)

aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']})
