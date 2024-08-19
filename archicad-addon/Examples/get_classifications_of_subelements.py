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

response = ExecuteTapirCommand ('GetSelectedElements')
response = ExecuteTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements' : response['elements']
    })

cWallFrames = [subelement for subelements in response['subelementsOfHierarchicalElements'] for subelement in subelements['cWallFrames']]
classificationSystems = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']

response = ExecuteTapirCommand ('GetClassificationsOfElements', {
        'elements' : [{
            'elementId': subelement['elementId']
        } for subelement in cWallFrames],
        'classificationSystemIds' : [{
            'classificationSystemId': classificationSystem['classificationSystemId']
        } for classificationSystem in classificationSystems]
    })