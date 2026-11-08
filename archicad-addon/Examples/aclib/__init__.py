import http.client
import json
import os
import socket
import sys
import urllib.error
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

# Whether a failed command raises TapirCommandError instead of printing the error and returning
# None. Off by default, as the examples and the recorded test outputs print their errors;
# recommended for new scripts (aclib.raiseOnError = True), so that a script stops at the first
# failure. RunCommand and RunTapirCommand also take raiseOnError per call.
raiseOnError = False

# Seconds to wait for Archicad's answer; None waits as long as it takes (some commands run for
# long). RunCommand and RunTapirCommand also take timeout per call. A timeout raises
# TapirConnectionError.
timeout = None

# The ports Archicad's JSON server uses: the first Archicad takes 19723, the next ones the next
# free port (the same range as the official archicad package's ACConnection.connect).
archicadPorts = range (19723, 19744)

releasesUrl = 'https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest'


class TapirError (Exception):
    """Base class of the errors aclib raises."""


class TapirConnectionError (TapirError):
    """Archicad does not answer: it is not running, no project is open, or it did not answer in time."""

    def __init__ (self, port, reason = None):
        self.port = port
        self.reason = reason
        super ().__init__ (
            'Cannot reach Archicad on port {}. Start Archicad, open a project, and check that the Tapir Add-On '
            'is loaded (Options > Add-On Manager). If you run several Archicads, pass --port or use '
            'aclib.FindArchicadPorts ().'.format (port))


class TapirNotInstalledError (TapirError):
    """Archicad answers, but the Tapir Add-On is not loaded in it."""

    def __init__ (self, port):
        self.port = port
        super ().__init__ (
            'Archicad answers on port {}, but the Tapir Add-On is not loaded in it. Install it with the Tapir '
            'Installer from {} and check that it is enabled in Options > Add-On Manager.'.format (port, releasesUrl))


class TapirCommandError (TapirError):
    """A command answered with an error (raised only with raiseOnError)."""

    def __init__ (self, command, error):
        self.command = command
        self.error = error if isinstance (error, dict) else {'message': error}
        self.code = self.error.get ('code')
        self.message = self.error.get ('message')
        text = '{} failed'.format (command)
        if self.code is not None:
            text += ' (error code {})'.format (self.code)
        if self.message:
            text += ': {}'.format (self.message)
        super ().__init__ (text)


def _PrintShortTapirErrors (excType, excValue, excTraceback):
    # A missing Archicad or Tapir is not a bug of the script: say what to do, without a traceback.
    if issubclass (excType, TapirError) and not issubclass (excType, TapirCommandError):
        print ('Error: {}'.format (excValue), file = sys.stderr)
        return
    _previousExceptHook (excType, excValue, excTraceback)

_previousExceptHook = sys.excepthook
sys.excepthook = _PrintShortTapirErrors


def _Send (command, parameters, toPort, requestTimeout):
    """Sends one JSON command and returns Archicad's parsed answer; raises TapirConnectionError when it does not answer."""
    request = urllib.request.Request ('{}:{}'.format (host, toPort))
    request.add_header ('Content-Type', 'application/json')
    requestData = json.dumps ({'command': command, 'parameters': parameters}).encode ('utf8')
    try:
        if requestTimeout is None:
            response = urllib.request.urlopen (request, requestData)
        else:
            response = urllib.request.urlopen (request, requestData, timeout = requestTimeout)
        with response:
            return json.loads (response.read ())
    except (urllib.error.URLError, http.client.HTTPException, socket.timeout, TimeoutError, ConnectionError, ValueError) as exception:
        # HTTPException and ValueError (an answer that is not JSON): something else than Archicad uses the port.
        raise TapirConnectionError (toPort, exception) from exception


def _TapirCommandRequest (command, parameters):
    return {
        'addOnCommandId': {
            'commandNamespace': 'TapirCommand',
            'commandName': command
        },
        'addOnCommandParameters': parameters
    }


def _GetTapirVersion (toPort, requestTimeout):
    """The version of the Tapir Add-On answering on toPort, or None when Archicad answers without Tapir."""
    response = _Send ('API.ExecuteAddOnCommand', _TapirCommandRequest ('GetAddOnVersion', {}), toPort, requestTimeout)
    if not isinstance (response, dict) or not response.get ('succeeded'):
        return None
    addOnResponse = (response.get ('result') or {}).get ('addOnCommandResponse') or {}
    version = addOnResponse.get ('version')
    return version if version else None


def _ErrorOf (response):
    error = response.get ('error') if isinstance (response, dict) else None
    return error if error is not None else {'message': 'No result in the answer.'}


def RunCommand (command, parameters = None, debug = False, raiseOnError = None, timeout = None):
    if parameters is None:
        parameters = {}
    if not isinstance(parameters, dict):
        print ('Error:\nCommand parameters must be a dictionary. The given parameters:\n{}'.format (JsonDumpDictionary (parameters)))
    raiseErrors = _ShouldRaise (raiseOnError)

    if debug:
        print ('Command: ' + command)
        print ('Parameters:\n' + JsonDumpDictionary (parameters))

    response_json = _Send (command, parameters, port, _TimeoutOf (timeout))
    return _ResultOf (command, response_json, debug, raiseErrors)


def _ResultOf (command, response_json, debug, raiseErrors):
    failed = 'error' in response_json or not response_json.get ('succeeded') or 'result' not in response_json
    if raiseErrors and failed:
        raise TapirCommandError (command, _ErrorOf (response_json))

    if 'error' in response_json:
        print ('Error:\n{}'.format (JsonDumpDictionary (response_json['error'])))

    if 'succeeded' not in response_json or 'result' not in response_json:
        return None

    result = response_json['result'] if response_json['succeeded'] else None
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    return result


# The commands that address a Script UI window. A script started from the Tapir palette has its
# own id in the TAPIR_SCRIPT_ID environment variable; RunTapirCommand passes it to these commands
# as scriptId, unless the call gives uiId or scriptId, so that the script uses its own window.
_scriptUICommands = ('ShowScriptUI', 'GetScriptUIResult')


def _WithScriptId (command, parameters):
    scriptId = os.environ.get ('TAPIR_SCRIPT_ID')
    if not scriptId or command not in _scriptUICommands or not isinstance (parameters, dict):
        return parameters
    if 'uiId' in parameters or 'scriptId' in parameters:
        return parameters
    return dict (parameters, scriptId = scriptId)


def RunTapirCommand (command, parameters = None, debug = None, raiseOnError = None, timeout = None):
    if debug is None:
        debug = printCommands
    if parameters is None:
        parameters = {}
    parameters = _WithScriptId (command, parameters)
    raiseErrors = _ShouldRaise (raiseOnError)
    requestTimeout = _TimeoutOf (timeout)

    if debug:
        print ('Command: ' + command)
        print ('Parameters:\n' + JsonDumpDictionary (parameters))

    response_json = _Send ('API.ExecuteAddOnCommand', _TapirCommandRequest (command, parameters), port, requestTimeout)
    if not response_json.get ('succeeded'):
        # Archicad refused the command: either Tapir is not loaded (GetAddOnVersion fails too) or the
        # command itself was refused (an unknown command of an older Tapir, invalid parameters).
        if _GetTapirVersion (port, requestTimeout) is None:
            raise TapirNotInstalledError (port)

    commandResult = _ResultOf (command, response_json, False, raiseErrors)

    result = None if commandResult == None else commandResult['addOnCommandResponse']
    if debug:
        print ('Response:\n' + JsonDumpDictionary (result))

    if result and 'error' in result:
        if raiseErrors:
            raise TapirCommandError (command, result['error'])
        print ('Error:\n{}'.format (JsonDumpDictionary (result['error'])))

    return result


def FindArchicadPorts (probeTimeout = 1.0):
    """The Archicads that answer on this computer, in port order: [{'port': 19723, 'tapirVersion': '1.8.1'}, ...].

    tapirVersion is None for an Archicad without the Tapir Add-On. Each port of archicadPorts is
    asked for Tapir's version, waiting at most probeTimeout seconds, so it returns at once when no
    Archicad runs."""
    found = []
    for candidatePort in archicadPorts:
        try:
            version = _GetTapirVersion (candidatePort, probeTimeout)
        except TapirConnectionError:
            continue
        found.append ({'port': candidatePort, 'tapirVersion': version})
    return found


def _VersionParts (version):
    parts = []
    for part in str (version).strip ().split ('.'):
        digits = ''
        for character in part:
            if not character.isdigit ():
                break
            digits += character
        parts.append (int (digits) if digits else 0)
    while len (parts) > 1 and parts[-1] == 0:
        parts.pop ()
    return parts


def RequireTapir (minVersion):
    """Checks that the Archicad on aclib.port runs Tapir minVersion or newer and returns the installed version.

    Raises TapirConnectionError or TapirNotInstalledError when Archicad or Tapir is missing, and
    TapirError when the installed Tapir is older than minVersion."""
    installedVersion = _GetTapirVersion (port, _TimeoutOf (None))
    if installedVersion is None:
        raise TapirNotInstalledError (port)
    if _VersionParts (installedVersion) < _VersionParts (minVersion):
        raise TapirError ('This script needs Tapir {}, you have {}. Update the Tapir Add-On from {} '
                          '(or with the update button of the Tapir Palette).'.format (minVersion, installedVersion, releasesUrl))
    return installedVersion


def _ShouldRaise (raiseOnErrorArgument):
    return raiseOnError if raiseOnErrorArgument is None else raiseOnErrorArgument


def _TimeoutOf (timeoutArgument):
    return timeout if timeoutArgument is None else timeoutArgument


def JsonDumpDictionary (d):
    return json.dumps (d, indent = 4)

def PrintJson (value):
    print (JsonDumpDictionary (value))
