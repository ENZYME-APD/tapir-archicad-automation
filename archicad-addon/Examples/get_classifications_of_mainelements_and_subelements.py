import json
import aclib

def ExecuteTapirCommand (commandName, commandParameters = {}):
    print ('Command: {}'.format (commandName))
    print ('Parameters:')
    print (json.dumps (commandParameters, indent = 4))
    response = aclib.RunTapirCommand (commandName, commandParameters)
    print ('Response:')
    print (json.dumps (response, indent = 4))
    return response

mainElements = ExecuteTapirCommand ('GetSelectedElements')['elements']
response = ExecuteTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements' : mainElements
    })

allSubElements = []
for subelementGroups in response['subelementsOfHierarchicalElements']:
    for subelements in subelementGroups.values():
        for subelement in subelements:
            if 'elementId' in subelement:
                allSubElements.append (subelement)

classificationSystems = [{'classificationSystemId': classificationSystem['classificationSystemId']} for classificationSystem in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']]

classificationsOfMainElements = ExecuteTapirCommand ('GetClassificationsOfElements', {
        'elements' : mainElements,
        'classificationSystemIds' : classificationSystems
    })

classificationsOfSubElements = ExecuteTapirCommand ('GetClassificationsOfElements', {
        'elements' : allSubElements,
        'classificationSystemIds' : classificationSystems
    })