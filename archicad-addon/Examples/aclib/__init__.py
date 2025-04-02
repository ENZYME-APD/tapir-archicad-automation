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

    if 'error' in response_json:
        print ('Error:\n{}'.format (JsonDumpDictionary (response_json['error'])))

    if 'succeeded' not in response_json or 'result' not in response_json:
        return None

    return response_json['result'] if response_json['succeeded'] else None

def RunTapirCommand (command, parameters = {}, debug = True):
    if debug:
        print ('Command: ' + command)
        print ('Parameters:\n' + JsonDumpDictionary (parameters))

    commandResult = RunCommand ('API.ExecuteAddOnCommand', {
        'addOnCommandId': {
            'commandNamespace': 'TapirCommand',
            'commandName': command
        },
        'addOnCommandParameters': parameters
    })

    result = None if commandResult == None else commandResult['addOnCommandResponse']
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    if result and 'error' in result:
        print ('Error:\n{}'.format (JsonDumpDictionary (result['error'])))

    return result

def JsonDumpDictionary (d):
    return json.dumps (d, indent = 4)
