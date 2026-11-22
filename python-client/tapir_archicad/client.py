"""Client: sends Tapir commands to Archicad's JSON interface. Standard library only."""

import json
import urllib.error
import urllib.request

from .commands import Commands


class TapirError (Exception):
    """A Tapir command failed, or Archicad did not answer.

    code is Archicad's or Tapir's error code (None when there is none), message its text."""

    def __init__ (self, message, code = None, command = None):
        self.code = code
        self.message = message
        self.command = command
        text = message if command is None else '{}: {}'.format (command, message)
        if code is not None:
            text += ' (error code {})'.format (code)
        super ().__init__ (text)


def _ErrorOf (answer):
    error = answer.get ('error') if isinstance (answer, dict) else None
    if isinstance (error, dict):
        return error.get ('message') or json.dumps (error), error.get ('code')
    if error is not None:
        return str (error), None
    return 'No result in the answer.', None


class Client (Commands):
    """Runs Tapir commands in the Archicad that listens on host:port.

    Every Tapir command is a method named in snake_case (GetAddOnVersion -> get_add_on_version)
    whose keyword arguments are the command's parameters, also in snake_case. Arguments left at
    None are not sent. A method returns the command's response as a dict. When Archicad refuses
    the command, or the response is an error, it raises TapirError. The executionResults of
    commands that work on several items are returned as they are: check them yourself.

    The first Archicad listens on port 19723, the next ones on the following ports."""

    def __init__ (self, host = 'http://127.0.0.1', port = 19723, timeout = None):
        self.host = host
        self.port = port
        self.timeout = timeout
        # Archicad answers on the machine the client runs on, so a request for it must never go
        # through http_proxy: a corporate proxy answers 407, or refuses the loopback address, and
        # that reached the caller as "Archicad does not answer".
        self._opener = urllib.request.build_opener (urllib.request.ProxyHandler ({}))

    @property
    def url (self):
        return '{}:{}'.format (self.host, self.port)

    def post (self, command, parameters):
        """Sends one command of Archicad's JSON interface and returns its parsed answer."""
        data = json.dumps ({'command': command, 'parameters': parameters}).encode ('utf-8')
        request = urllib.request.Request (self.url, data = data, headers = {'Content-Type': 'application/json'})
        try:
            with self._opener.open (request, timeout = self.timeout) as response:
                return json.loads (response.read ().decode ('utf-8'))
        except urllib.error.HTTPError as exception:
            # Archicad's JSON interface answers an error status with its own JSON error in the
            # body, in the same shape a successful answer has; returning it keeps the code and the
            # message the caller needs, which the status alone does not carry.
            return self._AnswerOfHTTPError (exception, command)
        except (urllib.error.URLError, OSError) as exception:
            raise TapirError ('Archicad does not answer on {}: {}'.format (self.url, exception), command = command) from exception
        except ValueError as exception:
            raise TapirError ('Archicad answered on {} with something that is not JSON: {}'.format (self.url, exception), command = command) from exception

    def _AnswerOfHTTPError (self, exception, command):
        """The answer inside an HTTP error response: its JSON body, as post returns it.

        A body that is not JSON (a proxy's page, a firewall's, a page login) is not an Archicad
        answer, so it raises, naming the status and the beginning of the body."""
        try:
            body = exception.read ().decode ('utf-8', 'replace')
        except (OSError, ValueError):
            body = ''
        finally:
            exception.close ()
        try:
            answer = json.loads (body)
        except ValueError:
            answer = None
        if isinstance (answer, dict):
            return answer
        beginning = ' '.join (body.split ())[:200]
        raise TapirError ('Archicad answered HTTP {} on {}{}'.format (
            exception.code,
            self.url,
            ': ' + beginning if beginning else ''), command = command) from exception

    def run (self, command, parameters = None):
        """Runs the Tapir command named command with the given parameters (a dict) and returns its response."""
        answer = self.post ('API.ExecuteAddOnCommand', {
            'addOnCommandId': {'commandNamespace': 'TapirCommand', 'commandName': command},
            'addOnCommandParameters': parameters if parameters is not None else {}
        })
        if not isinstance (answer, dict) or not answer.get ('succeeded') or 'error' in answer:
            message, code = _ErrorOf (answer)
            raise TapirError (message, code, command)
        response = (answer.get ('result') or {}).get ('addOnCommandResponse')
        if isinstance (response, dict) and 'error' in response:
            message, code = _ErrorOf (response)
            raise TapirError (message, code, command)
        return response
