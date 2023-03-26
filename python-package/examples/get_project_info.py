import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))
from tapir_py import core

ac = core.Command.create()
projectInfo = ac.GetProjectInfo()
print ('isUntitled: {0}'.format(projectInfo.isUntitled))
print ('isTeamwork: {0}'.format(projectInfo.isTeamwork))
if not projectInfo.isUntitled:
    print ('projectName: {0}'.format(projectInfo.projectName))
    print ('projectPath: {0}'.format(projectInfo.projectPath))
    print ('projectLocation: {0}'.format(projectInfo.projectLocation))
