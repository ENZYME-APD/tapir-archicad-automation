import json
import urllib.request
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--host', dest='host', type=str, default='http://127.0.0.1')
parser.add_argument('--port', dest='port', type=int, default=19723)
args, unknownArgs = parser.parse_known_args()
host = 'http://127.0.0.1' if 'host' not in args else args.host
port = 19723 if 'port' not in args else args.port

# Whether RunTapirCommand prints each command, its parameters and its response when the call
# does not say (debug = True or False). Off for the examples, which print what they want to
# show; the test scripts and workflows turn it on (see their aclib.py). RunCommand prints only
# with debug = True.
printCommands = False

def RunCommand (command, parameters = None, debug = False, printErrors = True):
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

    if 'error' in response_json and printErrors:
        print ('Error:\n{}'.format (JsonDumpDictionary (response_json['error'])))

    if 'succeeded' not in response_json or 'result' not in response_json:
        return None

    result = response_json['result'] if response_json['succeeded'] else None
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    return result

def RunTapirCommand (command, parameters = None, debug = None):
    if debug is None:
        debug = printCommands
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

# The command names of the installed Tapir, read once by HasCommand: None before the first call,
# False when the installed Tapir is too old to have GetCommands.
_tapirCommandNames = None

def _VersionTuple (version):
    return tuple (int (part) for part in str (version).split ('.') if part.isdigit ())

def HasCommand (name, introducedIn = None):
    """True when the installed Tapir Add-On has the command name, else False.

    Asks GetCommands once per process. A Tapir too old to have GetCommands is compared by its
    version (GetAddOnVersion) with introducedIn, the Tapir version the command was introduced in
    (as in the command reference); without introducedIn the answer is False with a message."""
    global _tapirCommandNames
    if _tapirCommandNames is None:
        result = RunCommand ('API.ExecuteAddOnCommand', {
            'addOnCommandId': {'commandNamespace': 'TapirCommand', 'commandName': 'GetCommands'},
            'addOnCommandParameters': {}
        }, printErrors = False)
        response = result.get ('addOnCommandResponse') if isinstance (result, dict) else None
        if isinstance (response, dict) and isinstance (response.get ('commands'), list):
            _tapirCommandNames = {command['name'] for command in response['commands']}
        else:
            _tapirCommandNames = False
    if _tapirCommandNames is not False:
        return name in _tapirCommandNames

    if introducedIn is not None:
        versionResult = RunTapirCommand ('GetAddOnVersion')
        if versionResult and 'version' in versionResult:
            return _VersionTuple (versionResult['version']) >= _VersionTuple (introducedIn)
    print ('The installed Tapir Add-On cannot list its commands, so {} is taken as missing. Update Tapir to the latest version.'.format (name))
    return False

def JsonDumpDictionary (d):
    return json.dumps (d, indent = 4)

def PrintJson (value):
    print (JsonDumpDictionary (value))
