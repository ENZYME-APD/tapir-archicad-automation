"""Exports every issue of the project to a BCF file in the temporary folder with ExportIssuesToBCF.

Leaving out the issues parameter, as here, exports every issue of the project.
"""
import os
import tempfile
import aclib

exportPath = os.path.join (tempfile.mkdtemp (prefix = 'TapirBCF_'), 'issues.bcfzip')

aclib.PrintJson (aclib.RunTapirCommand ('ExportIssuesToBCF', {
    'exportPath': exportPath,
    'useExternalId': False,
    'alignBySurveyPoint': True
}))
print ('Exported to', exportPath)
