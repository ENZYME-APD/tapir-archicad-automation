"""Lists the elements attached to each issue of the project, by attachment type, with GetElementsAttachedToIssue.

Needs at least one issue.
"""
import aclib

issues = aclib.RunTapirCommand ('GetIssues')['issues']
if not issues:
    raise SystemExit ('No issue in the project.')

for issue in issues:
    for attachmentType in ['Creation', 'Highlight', 'Deletion', 'Modification']:
        aclib.PrintJson (aclib.RunTapirCommand ('GetElementsAttachedToIssue', {'issueId': issue['issueId'], 'type': attachmentType}))
