import aclib
import tempfile
import os

newIssue = aclib.RunTapirCommand (
    'CreateIssue', {
        'name': 'Just a sample issue', 'tagText': 'python'
    })

newIssue2 = aclib.RunTapirCommand (
    'CreateIssue', {
        'name': 'Just a sample issue #2', 'tagText': 'python #2'
    })

issues = aclib.RunTapirCommand ('GetIssues', {})

aclib.RunTapirCommand (
    'AddCommentToIssue', {
        'issueId': newIssue['issueId'],
        'text': 'dropped da comment here',
        'author': 'python',
        'status': 'Info'
    })

# Archicad cores when using GetCommentsFromIssue after CreateIssue command, but it's an Archicad bug
# comments = aclib.RunTapirCommand ('GetCommentsFromIssue', {'issueId': newIssue['issueId']})

for type, elementType in [('Creation', 'Wall'),
                          ('Highlight', 'Object'),
                          ('Deletion', 'Column'),
                          ('Modification', 'Slab')]:
    aclib.RunTapirCommand (
        'AttachElementsToIssue', {
            'issueId': newIssue['issueId'],
            'elements': aclib.RunCommand ('API.GetElementsByType', {'elementType': elementType})['elements'],
            'type': type
        })

for type in ['Creation', 'Highlight', 'Deletion', 'Modification']:
    attachedElements = aclib.RunTapirCommand (
        'GetElementsAttachedToIssue', {
            'issueId': newIssue['issueId'],
            'type': type
        })

exportedFilePath = os.path.join (tempfile.gettempdir (), 'issues_test.bcfzip')
aclib.RunTapirCommand (
    'ExportIssuesToBCF', {
        'issues': [newIssue, newIssue2],
        'exportPath': exportedFilePath,
        'useExternalId': False,
        'alignBySurveyPoint': True
    })

aclib.RunTapirCommand (
    'DetachElementsFromIssue', {
        'issueId': newIssue['issueId'],
        'elements': aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Object'})['elements']
    })

for type in ['Creation', 'Highlight', 'Deletion', 'Modification']:
    attachedElements = aclib.RunTapirCommand (
        'GetElementsAttachedToIssue', {
            'issueId': newIssue['issueId'],
            'type': type
        })

aclib.RunTapirCommand ('DeleteIssue', newIssue)
aclib.RunTapirCommand ('DeleteIssue', newIssue2)

issues = aclib.RunTapirCommand ('GetIssues', {})

aclib.RunTapirCommand (
    'ImportIssuesFromBCF', {
        'importPath': exportedFilePath,
        'alignBySurveyPoint': True
    })

issues = aclib.RunTapirCommand ('GetIssues', {})