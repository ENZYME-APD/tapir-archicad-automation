"""Changes the description of the first revision issue that is not issued yet with ModifyRevisionIssues, then restores it.

Needs a revision issue that is not issued.
"""
import aclib

revisionIssues = aclib.RunTapirCommand ('GetRevisionIssues')['revisionIssues']
openIssues = [issue for issue in revisionIssues if not issue['isIssued']]
if not openIssues:
    raise SystemExit ('No revision issue that is not issued yet.')
revisionIssue = openIssues[0]

# Only the given fields change.
aclib.PrintJson (aclib.RunTapirCommand ('ModifyRevisionIssues', {
    'revisionIssues': [{
        'revisionIssueId': revisionIssue['revisionIssueId'],
        'description': revisionIssue['description'] + ' (Tapir)'
    }]
}))
aclib.RunTapirCommand ('GetRevisionIssues')

aclib.PrintJson (aclib.RunTapirCommand ('ModifyRevisionIssues', {
    'revisionIssues': [{
        'revisionIssueId': revisionIssue['revisionIssueId'],
        'description': revisionIssue['description']
    }]
}))
