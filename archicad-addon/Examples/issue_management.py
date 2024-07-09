import json
import aclib

commandName = 'CreateIssue'
commandParameters = {'name': 'Just a sample issue', 'tagText': 'python'}

print ('Command: {commandName}'.format (commandName = commandName))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = aclib.RunTapirCommand (commandName, commandParameters)
print ('Response:')
print (json.dumps (response, indent = 4))


# commandName = 'DeleteIssue'
# commandParameters = {'issueId': '4765CCCA-D30E-4A46-92B3-F544F936E089"'}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'GetIssues'
# commandParameters = {}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'AddComment'
# commandParameters = {'issueId': 'B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 'text': 'dropped da comment here', 'author': 'python', 'status': 2}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'GetComments'
# commandParameters = {'issueId': 'B0D7E9D6-56E8-4432-AC74-DE7C3083015F'}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'AttachElements'
# commandParameters = {'issueId': 'B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 'elementsIds': ['22F534EF-F3CE-4A99-9E54-125E899E6AA2',], 'type': 0}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'DetachElements'
# commandParameters = {'issueId': 'B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 'elementsIds': ['22F534EF-F3CE-4A99-9E54-125E899E6AA2',]}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'GetAttachedElements'
# commandParameters = {'issueId': 'B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 'type': 1}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'ExportToBCF'
# commandParameters = {'issuesIds': ['B0D7E9D6-56E8-4432-AC74-DE7C3083015F', '50D45AAD-9581-4275-A503-4E82C974C03B'], 'exportPath': 'C:\\Users\\i.yurasov\\Desktop\\dev\\issues_test6.bcfzip', 'useExternalId': False, 'alignBySurveyPoint': True}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))


# commandName = 'ImportFromBCF'
# commandParameters = {'importPath': 'C:\\Users\\i.yurasov\\Desktop\\dev\\issues_test5.bcfzip', 'alignBySurveyPoint': True}

# print ('Command: {commandName}'.format (commandName = commandName))
# print ('Parameters:')
# print (json.dumps (commandParameters, indent = 4))

# response = aclib.RunTapirCommand (commandName, commandParameters)
# print ('Response:')
# print (json.dumps (response, indent = 4))






