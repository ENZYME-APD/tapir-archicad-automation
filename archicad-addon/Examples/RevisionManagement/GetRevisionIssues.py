"""Lists the revision issues of the project with GetRevisionIssues."""
import aclib

revisionIssues = aclib.RunTapirCommand ('GetRevisionIssues')['revisionIssues']

for revisionIssue in revisionIssues:
    print (revisionIssue['id'], revisionIssue['description'], '(issued)' if revisionIssue['isIssued'] else '')
