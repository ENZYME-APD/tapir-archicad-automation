"""Tests of Examples/aclib against a small local HTTP server standing in for Archicad.

No Archicad is needed: python archicad-addon/Test/test_aclib.py"""
import contextlib
import http.server
import importlib.util
import io
import json
import os
import socket
import sys
import threading
import time
import unittest

_path = os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'Examples', 'aclib', '__init__.py')
_spec = importlib.util.spec_from_file_location ('aclib_under_test', os.path.normpath (_path))
aclib = importlib.util.module_from_spec (_spec)
_spec.loader.exec_module (aclib)


class FakeArchicad:
    """Answers Archicad's JSON requests; tapirVersion None plays an Archicad without Tapir."""

    def __init__ (self, tapirVersion = '1.8.1', responses = None, delay = 0.0):
        self.tapirVersion = tapirVersion
        self.responses = responses or {}
        self.delay = delay
        self.requests = []
        fake = self

        class Handler (http.server.BaseHTTPRequestHandler):
            def do_POST (self):
                request = json.loads (self.rfile.read (int (self.headers['Content-Length'])))
                fake.requests.append (request)
                if fake.delay:
                    time.sleep (fake.delay)
                body = json.dumps (fake.Answer (request)).encode ('utf8')
                try:
                    self.send_response (200)
                    self.send_header ('Content-Type', 'application/json')
                    self.send_header ('Content-Length', str (len (body)))
                    self.end_headers ()
                    self.wfile.write (body)
                except OSError:
                    pass

            def log_message (self, *args):
                pass

        self.server = http.server.ThreadingHTTPServer (('127.0.0.1', 0), Handler)
        self.server.daemon_threads = True
        self.port = self.server.server_address[1]
        self.thread = threading.Thread (target = self.server.serve_forever, daemon = True)
        self.thread.start ()

    def Answer (self, request):
        if request['command'] != 'API.ExecuteAddOnCommand':
            return self.responses.get (request['command'], {'succeeded': True, 'result': {}})
        commandId = request['parameters']['addOnCommandId']
        if self.tapirVersion is None or commandId['commandNamespace'] != 'TapirCommand':
            return {'succeeded': False, 'error': {'code': 4001, 'message': 'Unknown add-on command.'}}
        name = commandId['commandName']
        if name == 'GetAddOnVersion':
            return {'succeeded': True, 'result': {'addOnCommandResponse': {'version': self.tapirVersion}}}
        if name in self.responses:
            return self.responses[name]
        return {'succeeded': False, 'error': {'code': 4001, 'message': 'Unknown add-on command.'}}

    def Close (self):
        self.server.shutdown ()
        self.server.server_close ()


def FreePort ():
    with socket.socket () as s:
        s.bind (('127.0.0.1', 0))
        return s.getsockname ()[1]


def TapirAnswer (response):
    return {'succeeded': True, 'result': {'addOnCommandResponse': response}}


class AclibTest (unittest.TestCase):

    def setUp (self):
        self.saved = (aclib.port, aclib.host, aclib.raiseOnError, aclib.timeout, aclib.archicadPorts, aclib.printCommands)
        aclib.host = 'http://127.0.0.1'
        aclib.raiseOnError = False
        aclib.timeout = None
        aclib.printCommands = False
        self.servers = []

    def tearDown (self):
        aclib.port, aclib.host, aclib.raiseOnError, aclib.timeout, aclib.archicadPorts, aclib.printCommands = self.saved
        for server in self.servers:
            server.Close ()

    def Start (self, **kwargs):
        server = FakeArchicad (**kwargs)
        self.servers.append (server)
        aclib.port = server.port
        return server

    def Run (self, function, *args, **kwargs):
        output = io.StringIO ()
        with contextlib.redirect_stdout (output):
            result = function (*args, **kwargs)
        return result, output.getvalue ()

    def test_refused_connection_raises_connection_error (self):
        aclib.port = FreePort ()
        with self.assertRaises (aclib.TapirConnectionError) as context:
            aclib.RunTapirCommand ('GetAddOnVersion')
        self.assertIn ('Cannot reach Archicad on port {}.'.format (aclib.port), str (context.exception))
        self.assertIn ('Options > Add-On Manager', str (context.exception))
        self.assertIn ('aclib.FindArchicadPorts ()', str (context.exception))
        with self.assertRaises (aclib.TapirConnectionError):
            aclib.RunCommand ('API.IsAlive')

    def test_timeout_raises_connection_error (self):
        self.Start (delay = 1.0)
        aclib.timeout = 0.2
        with self.assertRaises (aclib.TapirConnectionError):
            aclib.RunTapirCommand ('GetAddOnVersion')
        aclib.timeout = None
        with self.assertRaises (aclib.TapirConnectionError):
            aclib.RunCommand ('API.IsAlive', timeout = 0.2)

    def test_success_is_unchanged (self):
        self.Start (responses = {'GetProjectInfo': TapirAnswer ({'projectName': 'P'})})
        result, output = self.Run (aclib.RunTapirCommand, 'GetProjectInfo')
        self.assertEqual (result, {'projectName': 'P'})
        self.assertEqual (output, '')
        result, output = self.Run (aclib.RunCommand, 'API.IsAlive')
        self.assertEqual (result, {})
        self.assertEqual (output, '')

    def test_tapir_not_loaded_raises_not_installed (self):
        self.Start (tapirVersion = None)
        with self.assertRaises (aclib.TapirNotInstalledError) as context:
            self.Run (aclib.RunTapirCommand, 'GetProjectInfo')
        self.assertIn ('Tapir Add-On is not loaded', str (context.exception))
        self.assertIn ('releases', str (context.exception))
        self.assertIsInstance (context.exception, aclib.TapirError)
        self.assertNotIsInstance (context.exception, aclib.TapirConnectionError)

    def test_unknown_command_with_tapir_loaded_prints_the_error (self):
        self.Start ()
        result, output = self.Run (aclib.RunTapirCommand, 'NoSuchCommand')
        self.assertIsNone (result)
        self.assertEqual (output, 'Error:\n{}\n'.format (json.dumps ({'code': 4001, 'message': 'Unknown add-on command.'}, indent = 4)))
        with self.assertRaises (aclib.TapirCommandError) as context:
            aclib.RunTapirCommand ('NoSuchCommand', raiseOnError = True)
        self.assertEqual (context.exception.code, 4001)

    def test_error_response_prints_and_returns_none_by_default (self):
        error = {'code': 2, 'message': 'Wrong element.'}
        self.Start (responses = {'Failing': TapirAnswer ({'success': False, 'error': error})})
        result, output = self.Run (aclib.RunTapirCommand, 'Failing')
        self.assertEqual (result, {'success': False, 'error': error})
        self.assertEqual (output, 'Error:\n{}\n'.format (json.dumps (error, indent = 4)))

    def test_error_response_raises_with_raise_on_error (self):
        error = {'code': 2, 'message': 'Wrong element.'}
        self.Start (responses = {'Failing': TapirAnswer ({'success': False, 'error': error}),
                                 'API.Failing': {'succeeded': False, 'error': {'code': 7, 'message': 'No.'}}})
        aclib.raiseOnError = True
        with self.assertRaises (aclib.TapirCommandError) as context:
            aclib.RunTapirCommand ('Failing')
        self.assertEqual ((context.exception.command, context.exception.code, context.exception.message),
                          ('Failing', 2, 'Wrong element.'))
        with self.assertRaises (aclib.TapirCommandError) as context:
            aclib.RunCommand ('API.Failing')
        self.assertEqual (context.exception.code, 7)
        # The per-call argument wins over the module flag, both ways.
        result, output = self.Run (aclib.RunTapirCommand, 'Failing', raiseOnError = False)
        self.assertEqual (result['error'], error)
        aclib.raiseOnError = False
        with self.assertRaises (aclib.TapirCommandError):
            aclib.RunTapirCommand ('Failing', raiseOnError = True)
        result, output = self.Run (aclib.RunCommand, 'API.Failing')
        self.assertIsNone (result)
        self.assertTrue (output.startswith ('Error:\n'))

    def test_find_archicad_ports (self):
        first = FakeArchicad (tapirVersion = '1.8.1')
        second = FakeArchicad (tapirVersion = None)
        self.servers += [first, second]
        closed = FreePort ()
        aclib.archicadPorts = [first.port, closed, second.port]
        started = time.time ()
        self.assertEqual (aclib.FindArchicadPorts (), [{'port': first.port, 'tapirVersion': '1.8.1'},
                                                       {'port': second.port, 'tapirVersion': None}])
        self.assertLess (time.time () - started, 5)
        aclib.archicadPorts = [FreePort ()]
        self.assertEqual (aclib.FindArchicadPorts (), [])

    def test_find_archicad_ports_skips_a_port_that_does_not_answer_in_time (self):
        slow = FakeArchicad (delay = 1.0)
        self.servers.append (slow)
        aclib.archicadPorts = [slow.port]
        self.assertEqual (aclib.FindArchicadPorts (probeTimeout = 0.2), [])

    def test_default_port_range (self):
        self.assertEqual ((aclib.archicadPorts[0], aclib.archicadPorts[-1]), (19723, 19743))

    def test_require_tapir (self):
        self.Start (tapirVersion = '1.10.0')
        self.assertEqual (aclib.RequireTapir ('1.9.2'), '1.10.0')
        self.assertEqual (aclib.RequireTapir ('1.10'), '1.10.0')
        with self.assertRaises (aclib.TapirError) as context:
            aclib.RequireTapir ('1.10.1')
        self.assertIn ('needs Tapir 1.10.1, you have 1.10.0', str (context.exception))
        with self.assertRaises (aclib.TapirError):
            aclib.RequireTapir ('2')

    def test_require_tapir_without_tapir_or_archicad (self):
        self.Start (tapirVersion = None)
        with self.assertRaises (aclib.TapirNotInstalledError):
            aclib.RequireTapir ('1.0.0')
        aclib.port = FreePort ()
        with self.assertRaises (aclib.TapirConnectionError):
            aclib.RequireTapir ('1.0.0')

    def test_uncaught_connection_error_prints_only_the_message (self):
        stderr = io.StringIO ()
        with contextlib.redirect_stderr (stderr):
            try:
                raise aclib.TapirConnectionError (19723)
            except aclib.TapirConnectionError:
                sys.excepthook (*sys.exc_info ())
        self.assertTrue (stderr.getvalue ().startswith ('Error: Cannot reach Archicad on port 19723.'))
        self.assertNotIn ('Traceback', stderr.getvalue ())


if __name__ == '__main__':
    unittest.main ()
