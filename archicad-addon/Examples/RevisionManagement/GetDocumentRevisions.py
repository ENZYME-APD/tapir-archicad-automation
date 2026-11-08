"""Lists the document revisions of the layouts with GetDocumentRevisions."""
import aclib

documentRevisions = aclib.RunTapirCommand ('GetDocumentRevisions')['documentRevisions']

for revision in documentRevisions:
    print (revision['layoutInfo']['name'], revision['finalId'], revision['status'])
