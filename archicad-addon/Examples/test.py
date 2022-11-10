import json
import urllib.request

command = 'GetSelectedElements'
commandParameters = {}

host = 'http://127.0.0.1'
port = '19723'

connection_object = urllib.request.Request('{}:{}'.format(host, port))
connection_object.add_header('Content-Type', 'application/json')

data = {
    'command' : 'API.ExecuteAddOnCommand',
    'parameters': {
        'addOnCommandId': {
            'commandNamespace': 'TapirCommand',
            'commandName': command
        },
        'addOnCommandParameters': commandParameters
    }    
}

print ('Command: {command}'.format (command = command))
print ('Parameters:')
print (json.dumps (commandParameters, indent = 4))

response = urllib.request.urlopen(connection_object, json.dumps(data).encode('utf8'))
responseJson = json.loads (response.read())
print ('Response:')
print (json.dumps (responseJson, indent = 4))
