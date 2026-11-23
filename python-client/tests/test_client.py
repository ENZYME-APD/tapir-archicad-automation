"""Tests of the generated Python client; no Archicad needed.

    python -m unittest discover python-client/tests
"""

import http.server
import inspect
import json
import os
import pathlib
import sys
import threading
import unittest
import urllib.request

CLIENT_ROOT = pathlib.Path (__file__).resolve ().parent.parent
sys.path.insert (0, str (CLIENT_ROOT))

import generate  # noqa: E402
import tapir_archicad  # noqa: E402
from tapir_archicad.commands import Commands  # noqa: E402


class GeneratedFileTest (unittest.TestCase):

    def test_committed_commands_py_is_what_generate_py_writes (self):
        committed = generate.OUTPUT.read_text (encoding = 'utf-8')
        self.assertTrue (committed == generate.RenderFromFiles (),
            'tapir_archicad/commands.py is out of date: run python python-client/generate.py')

    def test_one_method_per_command (self):
        commands = [command for group in generate.LoadJsVariable (generate.COMMANDS_JS) for command in group['commands']]
        for command in commands:
            method = getattr (Commands, generate.SnakeCase (command['name']), None)
            self.assertTrue (callable (method), command['name'])
        self.assertEqual (len (commands), len ([name for name, _ in inspect.getmembers (Commands, inspect.isfunction) if name != 'run' and not name.startswith ('_')]))

    def test_snake_case (self):
        self.assertEqual (generate.SnakeCase ('GetAddOnVersion'), 'get_add_on_version')
        self.assertEqual (generate.SnakeCase ('GetIFCTypeOfElements'), 'get_ifc_type_of_elements')
        self.assertEqual (generate.SnakeCase ('Set3DProjectionSettings'), 'set_3d_projection_settings')
        self.assertEqual (generate.SnakeCase ('elements'), 'elements')
        self.assertEqual (generate.SnakeCase ('navigatorItemId'), 'navigator_item_id')
        self.assertEqual (generate.SnakeCase ('from'), 'from_')

    def test_required_arguments_have_no_default (self):
        parameters = inspect.signature (Commands.get_containing_databases).parameters
        self.assertIs (parameters['elements'].default, inspect.Parameter.empty)
        self.assertEqual (parameters['elements'].kind, inspect.Parameter.KEYWORD_ONLY)
        parameters = inspect.signature (Commands.get_details_of_elements).parameters
        self.assertIsNone (parameters['fields'].default)


class FakeArchicad (http.server.BaseHTTPRequestHandler):
    """Answers every request with FakeArchicad.answer, with FakeArchicad.status, or with
    FakeArchicad.body as it is when that is not None (a page that is not JSON, say), and keeps the
    requests it got."""
    answer = {}
    body = None
    status = 200
    requests = []

    def do_POST (self):
        length = int (self.headers['Content-Length'])
        FakeArchicad.requests.append (json.loads (self.rfile.read (length)))
        if FakeArchicad.body is None:
            body = json.dumps (FakeArchicad.answer).encode ('utf-8')
        else:
            body = FakeArchicad.body.encode ('utf-8')
        self.send_response (FakeArchicad.status)
        self.send_header ('Content-Type', 'application/json')
        self.send_header ('Content-Length', str (len (body)))
        self.end_headers ()
        self.wfile.write (body)

    def log_message (self, *args):
        pass


class ClientTest (unittest.TestCase):

    @classmethod
    def setUpClass (cls):
        cls.server = http.server.HTTPServer (('127.0.0.1', 0), FakeArchicad)
        cls.thread = threading.Thread (target = cls.server.serve_forever, daemon = True)
        cls.thread.start ()
        cls.client = tapir_archicad.Client (port = cls.server.server_address[1], timeout = 10)

    @classmethod
    def tearDownClass (cls):
        cls.server.shutdown ()
        cls.server.server_close ()

    def setUp (self):
        FakeArchicad.requests = []
        FakeArchicad.answer = {}
        FakeArchicad.body = None
        FakeArchicad.status = 200

    def test_request_and_response (self):
        FakeArchicad.answer = {'succeeded': True, 'result': {'addOnCommandResponse': {'version': '2.0.1'}}}
        self.assertEqual (self.client.get_add_on_version (), {'version': '2.0.1'})
        self.assertEqual (FakeArchicad.requests, [{
            'command': 'API.ExecuteAddOnCommand',
            'parameters': {
                'addOnCommandId': {'commandNamespace': 'TapirCommand', 'commandName': 'GetAddOnVersion'},
                'addOnCommandParameters': {}
            }
        }])

    def test_arguments_are_sent_by_their_json_names_and_none_is_left_out (self):
        FakeArchicad.answer = {'succeeded': True, 'result': {'addOnCommandResponse': {}}}
        self.client.change_window (window_type = 'FloorPlan', story_index = 1)
        self.assertEqual (FakeArchicad.requests[0]['parameters']['addOnCommandParameters'], {'windowType': 'FloorPlan', 'storyIndex': 1})

    def test_execution_results_are_returned_as_they_are (self):
        results = {'executionResults': [{'success': True}, {'success': False, 'error': {'code': 1, 'message': 'no'}}]}
        FakeArchicad.answer = {'succeeded': True, 'result': {'addOnCommandResponse': results}}
        self.assertEqual (self.client.get_containing_databases (elements = []), results)

    def test_error_in_the_response_raises (self):
        FakeArchicad.answer = {'succeeded': True, 'result': {'addOnCommandResponse': {'error': {'code': 42, 'message': 'Invalid input'}}}}
        with self.assertRaises (tapir_archicad.TapirError) as raised:
            self.client.get_add_on_version ()
        self.assertEqual ((raised.exception.code, raised.exception.message, raised.exception.command), (42, 'Invalid input', 'GetAddOnVersion'))

    def test_refused_command_raises (self):
        FakeArchicad.answer = {'succeeded': False, 'error': {'code': 4001, 'message': 'Unknown command'}}
        with self.assertRaises (tapir_archicad.TapirError) as raised:
            self.client.run ('NoSuchCommand', {})
        self.assertEqual ((raised.exception.code, raised.exception.message), (4001, 'Unknown command'))

    def test_http_error_answer_keeps_the_error_code_and_message (self):
        # Archicad answers a refused command with an HTTP error status and its own JSON error in
        # the body; the caller needs the code and the message, not "Archicad does not answer".
        FakeArchicad.status = 500
        FakeArchicad.answer = {'succeeded': False, 'error': {'code': 4001, 'message': 'Unknown command'}}
        with self.assertRaises (tapir_archicad.TapirError) as raised:
            self.client.run ('NoSuchCommand', {})
        self.assertEqual ((raised.exception.code, raised.exception.message), (4001, 'Unknown command'))

    def test_http_error_body_that_is_not_json_raises_with_the_status (self):
        # A proxy or a firewall answers instead of Archicad: say so, and quote what it said.
        FakeArchicad.status = 502
        FakeArchicad.body = '<html><body>Proxy authentication required</body></html>'
        with self.assertRaises (tapir_archicad.TapirError) as raised:
            self.client.get_add_on_version ()
        self.assertEqual (raised.exception.code, None)
        self.assertIn ('502', raised.exception.message)
        self.assertIn ('Proxy authentication required', raised.exception.message)

    def test_requests_ignore_the_proxy_environment_variables (self):
        # A BIM workstation usually has http_proxy set machine-wide; a request for 127.0.0.1 must
        # still reach Archicad, not the proxy.
        FakeArchicad.answer = {'succeeded': True, 'result': {'addOnCommandResponse': {'version': '2.0.1'}}}
        saved = {name: os.environ.get (name) for name in ('http_proxy', 'HTTP_PROXY', 'no_proxy', 'NO_PROXY')}
        savedOpener = urllib.request._opener
        try:
            for name in ('http_proxy', 'HTTP_PROXY'):
                os.environ[name] = 'http://127.0.0.1:1'
            for name in ('no_proxy', 'NO_PROXY'):
                os.environ.pop (name, None)
            # urlopen keeps the opener it built first in a module variable, with the proxies of that
            # moment; forgetting it makes the request read the environment as a new process does,
            # which is the case that broke.
            urllib.request._opener = None
            client = tapir_archicad.Client (port = self.server.server_address[1], timeout = 10)
            self.assertEqual (client.get_add_on_version (), {'version': '2.0.1'})
        finally:
            urllib.request._opener = savedOpener
            for name, value in saved.items ():
                if value is None:
                    os.environ.pop (name, None)
                else:
                    os.environ[name] = value

    def test_no_archicad_raises (self):
        port = self.server.server_address[1]
        closed = http.server.HTTPServer (('127.0.0.1', 0), FakeArchicad)
        unusedPort = closed.server_address[1]
        closed.server_close ()
        self.assertNotEqual (port, unusedPort)
        with self.assertRaises (tapir_archicad.TapirError):
            tapir_archicad.Client (port = unusedPort, timeout = 5).get_add_on_version ()


if __name__ == '__main__':
    unittest.main ()
