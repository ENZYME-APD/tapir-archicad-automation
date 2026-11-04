"""Creates an issue and adds a comment to it with AddCommentToIssue.

Deletes the issue at the end.
"""
import aclib

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue'})

aclib.PrintJson (aclib.RunTapirCommand ('AddCommentToIssue', {
    'issueId': issue['issueId'],
    'author': 'Tapir',
    'status': 'Info',
    'text': 'Comment added by the Tapir example.'
}))

aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']})
