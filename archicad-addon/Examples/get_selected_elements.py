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
