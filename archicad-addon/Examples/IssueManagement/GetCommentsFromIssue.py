"""Prints the comments of every issue of the project with GetCommentsFromIssue.

Needs at least one issue.
"""
import aclib

issues = aclib.RunTapirCommand ('GetIssues')['issues']
if not issues:
    raise SystemExit ('No issue in the project.')

for issue in issues:
    aclib.PrintJson (aclib.RunTapirCommand ('GetCommentsFromIssue', {'issueId': issue['issueId']}))
