import json
import urllib.request

host = 'http://127.0.0.1'
port = '19723'

def RunCommand (command, parameters):
    connection_object = urllib.request.Request ('{}:{}'.format (host, port))
    connection_object.add_header ('Content-Type', 'application/json')

    request_data = {
        'command' : command,
        'parameters': parameters
    }
    request_string = json.dumps (request_data).encode ('utf8')
    
    response_data = urllib.request.urlopen (connection_object, request_string)
    response_json = json.loads (response_data.read())
    
    if not response_json['succeeded']:
        return None
    
    return response_json['result']

def RunTapirCommand (command, parameters):
    commandResult = RunCommand ('API.ExecuteAddOnCommand', {
        'addOnCommandId': {
            'commandNamespace': 'TapirCommand',
            'commandName': command
        },
        'addOnCommandParameters': parameters
    })
    if commandResult == None:
        return None
    return commandResult['addOnCommandResponse']
