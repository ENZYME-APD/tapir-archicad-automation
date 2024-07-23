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

commandName = 'HighlightElements'
commandParameters = {
    'elements' : response['elements'],
    'highlightedColors' : [[(i*10) % 255, 0, 0, 255] for i in len(response['elements'])],
    'wireframe3D' : True,
    'nonHighlightedColor' : [0, 0, 255, 128]
}

print ('Command: {commandName}'.format (commandName = commandName))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))
