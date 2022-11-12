import json
import lib

commandName = 'GetSelectedElements'
commandParameters = {}

print ('Command: {commandName}'.format (commandName = commandName))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = lib.RunCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))
