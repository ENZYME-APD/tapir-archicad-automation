import json
import urllib.request
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--host', dest='host', type=str, default='http://127.0.0.1')
parser.add_argument('--port', dest='port', type=int, default=19723)
args, unknownArgs = parser.parse_known_args()
host = 'http://127.0.0.1' if 'host' not in args else args.host
port = 19723 if 'port' not in args else args.port

def RunCommand (command, parameters = None, debug = False):
    if parameters is None:
        parameters = {}
    if not isinstance(parameters, dict):
        print ('Error:\nCommand parameters must be a dictionary. The given parameters:\n{}'.format (JsonDumpDictionary (parameters)))

    if debug:
        print ('Command: ' + command)
        print ('Parameters:\n' + JsonDumpDictionary (parameters))

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

    result = response_json['result'] if response_json['succeeded'] else None
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    return result

def RunTapirCommand (command, parameters = None, debug = False):
    if parameters is None:
        parameters = {}

    if debug:
        print ('Command: ' + command)
        print ('Parameters:\n' + JsonDumpDictionary (parameters))

    commandResult = RunCommand ('API.ExecuteAddOnCommand', {
        'addOnCommandId': {
            'commandNamespace': 'TapirCommand',
            'commandName': command
        },
        'addOnCommandParameters': parameters
    }, debug = False)

    result = None if commandResult == None else commandResult['addOnCommandResponse']
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    if result and 'error' in result:
        print ('Error:\n{}'.format (JsonDumpDictionary (result['error'])))

    return result

def JsonDumpDictionary (d):
    return json.dumps (d, indent = 4)
