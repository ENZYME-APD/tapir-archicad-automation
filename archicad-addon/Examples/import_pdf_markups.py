import aclib
import os
import sys

# Imports the markups (comments) of a PDF published from Archicad into the Issue
# Manager, as Issue Manager > Import PDF markups does, without any dialog, and lists
# the issues the import created, so a script can process them (add comments, attach
# elements, export them to BCF, ...). Available from Archicad 29.
#
# Usage: python import_pdf_markups.py <path to the marked-up PDF>

pdfFilePath = next ((arg for arg in sys.argv[1:] if arg.lower ().endswith ('.pdf')), None)
if pdfFilePath is None:
    print ('Usage: python import_pdf_markups.py <path to the marked-up PDF>')
    sys.exit (0)

result = aclib.RunTapirCommand ('ImportMarkupsFromPDF', {
    'pdfFilePath': os.path.abspath (pdfFilePath)
}, debug = False)

executionResult = result['executionResult']
if not executionResult['success']:
    print ('Import failed: {}'.format (executionResult['error']['message']))
    sys.exit (1)

createdIssueIds = [issue['issueId']['guid'] for issue in result['issues']]
print ('Import succeeded, {} issue(s) created'.format (len (createdIssueIds)))

allIssues = aclib.RunTapirCommand ('GetIssues', {}, debug = False)['issues']
for issue in allIssues:
    if issue['issueId']['guid'] in createdIssueIds:
        print ('{}: {}'.format (issue['issueId']['guid'], issue['name']))
