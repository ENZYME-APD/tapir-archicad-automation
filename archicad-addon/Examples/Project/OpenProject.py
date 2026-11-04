"""Opens the open project's file again with OpenProject, which discards the unsaved changes.

Needs a saved, non-Teamwork project. Discards unsaved changes without asking.
"""
import aclib

projectInfo = aclib.RunTapirCommand ('GetProjectInfo')
if projectInfo['isTeamwork'] or 'projectPath' not in projectInfo:
    raise SystemExit ('Needs a saved, non-Teamwork project.')

aclib.PrintJson (aclib.RunTapirCommand ('OpenProject', {'projectFilePath': projectInfo['projectPath']}))
