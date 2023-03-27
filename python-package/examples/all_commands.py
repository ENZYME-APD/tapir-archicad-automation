import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))
from tapir_py import core

ac = core.Command.create()

print('IsAlive')
isAlive = ac.IsAlive()
print('\t{0}'.format(isAlive))

print('GetProductInfo')
productInfo = ac.GetProductInfo()
print('\t{0}'.format(productInfo))

print('GetProjectInfo')
projectInfo = ac.GetProjectInfo()
print('\tisUntitled: {0}'.format(projectInfo.isUntitled))
print('\tisTeamwork: {0}'.format(projectInfo.isTeamwork))
if not projectInfo.isUntitled:
    print('\tprojectName: {0}'.format(projectInfo.projectName))
    print('\tprojectPath: {0}'.format(projectInfo.projectPath))
    print('\tprojectLocation: {0}'.format(projectInfo.projectLocation))

print('GetAllElements')
allElements = ac.GetAllElements()
for element in allElements:
    print('\t{0}'.format(element.guid))

print('GetElementsByType')
wallElements = ac.GetElementsByType(core.Element.ELEMENT_TYPE.Wall)
for element in wallElements:
    print('\t{0}'.format(element.guid))

print('GetSelectedElements')
selectedElements = ac.GetSelectedElements()
for element in selectedElements:
    print('\t{0}'.format(element.guid))

print('GetAllClassificationSystems')
systems = ac.GetAllClassificationSystems()
for system in systems:
    print('\t{0}'.format(system.name))
    items = ac.GetAllClassificationsInSystem(system.guid)
    for item in items:
        print('\t{0}'.format(item.id))

print('Get2DBoundingBoxes')
boxes2D = ac.Get2DBoundingBoxes(wallElements)
for box2D in boxes2D:
    print('\t{0}'.format(box2D))

print('Get3DBoundingBoxes')
boxes3D = ac.Get3DBoundingBoxes(wallElements)
for box3D in boxes3D:
    print('\t{0}'.format(box3D))

print('GetAddOnVersion')
addOnVersion = ac.GetAddOnVersion()
print('\t{0}'.format(addOnVersion))

print('GetArchicadLocation')
archicadLocation = ac.GetArchicadLocation()
print('\t{0}'.format(archicadLocation))
