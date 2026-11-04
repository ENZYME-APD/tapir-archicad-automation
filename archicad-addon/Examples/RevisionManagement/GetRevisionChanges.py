"""Lists the revision changes of the project with GetRevisionChanges."""
import aclib

revisionChanges = aclib.RunTapirCommand ('GetRevisionChanges')['revisionChanges']

for change in revisionChanges:
    print (change['id'], change['description'], 'by', change['modifiedByUser'])
