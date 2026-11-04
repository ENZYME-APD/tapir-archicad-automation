"""Exports a new issue to a temporary BCF file, deletes it, then imports it back with ImportIssuesFromBCF.

Leaves the imported issue in the project.
"""
import os
import tempfile
import aclib

issue = aclib.RunTapirCommand ('CreateIssue', {'name': 'Tapir Example Issue'})
bcfPath = os.path.join (tempfile.mkdtemp (prefix = 'TapirBCF_'), 'issue.bcfzip')
aclib.RunTapirCommand ('ExportIssuesToBCF', {
    'issues': [{'issueId': issue['issueId']}],
    'exportPath': bcfPath,
    'useExternalId': False,
    'alignBySurveyPoint': True
})
aclib.RunTapirCommand ('DeleteIssue', {'issueId': issue['issueId']})

aclib.PrintJson (aclib.RunTapirCommand ('ImportIssuesFromBCF', {'importPath': bcfPath, 'alignBySurveyPoint': True}))
aclib.RunTapirCommand ('GetIssues')
