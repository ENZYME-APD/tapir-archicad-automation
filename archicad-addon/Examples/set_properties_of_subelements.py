import json
import aclib

commandName = 'GetSelectedElements'
commandParameters = {}

print ('Command: {commandName}'.format (commandName = commandName))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))

commandName = 'GetSubelementsOfHierarchicalElements'
commandParameters = {
    'hierarchicalElements' : response['elements']
}

print ('Command: {commandName}'.format (commandName = commandName))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [{"type": "BuiltIn", "nonLocalizedName": "General_ElementID"}]})['properties'][0]['propertyId']

cWallFrames = [subelement for subelements in response['subelementsOfHierarchicalElements'] for subelement in subelements['cWallFrames']]

commandName = 'GetPropertyValuesOfElements'
commandParameters = {
    'elements' : [{
        'elementId': subelement['elementId']
    } for subelement in cWallFrames],
    'properties' : [{
        'propertyId': elementIdPropertyId
    }]
}

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))

commandName = 'SetPropertyValuesOfElements'
commandParameters = {'elementPropertyValues' : []}

for i in range(len(cWallFrames)):
    previousElementIdOfCWallFrame = response['propertyValuesForElements'][i]['propertyValues'][0]['propertyValue']['value']
    newElementIdOfCWallFrame = 'NewID-{:04d} (PrevID: {})'.format(i, previousElementIdOfCWallFrame)

    commandParameters['elementPropertyValues'].append({
        'elementId': cWallFrames[i]['elementId'],
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': newElementIdOfCWallFrame}
    })

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))