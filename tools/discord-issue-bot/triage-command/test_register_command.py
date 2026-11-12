"""Offline tests of register_command.py: python -m unittest test_register_command"""
import io
import json
import unittest
from contextlib import redirect_stdout
from unittest import mock

import register_command as rc


class FakeResponse(io.BytesIO):
    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False


class RegisterCommandTests(unittest.TestCase):
    def test_definition_matches_what_the_worker_accepts(self):
        options = {o['name']: o for o in rc.TRIAGE_COMMAND['options']}
        self.assertEqual(rc.TRIAGE_COMMAND['name'], 'triage')
        self.assertEqual(set(options), {'dry_run', 'lookback_minutes'})
        self.assertEqual(options['lookback_minutes']['min_value'], 1)
        self.assertEqual(options['lookback_minutes']['max_value'], 10080)
        self.assertFalse(any(o['required'] for o in options.values()))

    def test_hidden_from_everyone_without_manage_server(self):
        self.assertEqual(rc.TRIAGE_COMMAND['default_member_permissions'], '32')
        self.assertFalse(rc.TRIAGE_COMMAND['dm_permission'])

    def test_register_puts_the_command_with_the_bot_token(self):
        seen = {}

        def opener(request, timeout):
            seen['url'] = request.full_url
            seen['method'] = request.get_method()
            seen['auth'] = request.get_header('Authorization')
            seen['body'] = json.loads(request.data)
            return FakeResponse(json.dumps([{'name': 'triage'}]).encode())

        result = rc.register('app1', 'guild1', 'tok', opener=opener)
        self.assertEqual(seen['url'], 'https://discord.com/api/v10/applications/app1/guilds/guild1/commands')
        self.assertEqual(seen['method'], 'PUT')
        self.assertEqual(seen['auth'], 'Bot tok')
        self.assertEqual(seen['body'], [rc.TRIAGE_COMMAND])
        self.assertEqual(result, [{'name': 'triage'}])

    def test_print_needs_no_token_and_makes_no_call(self):
        out = io.StringIO()
        with mock.patch.dict('os.environ', {}, clear=True), mock.patch('urllib.request.urlopen') as urlopen, redirect_stdout(out):
            self.assertEqual(rc.main(['--print']), 0)
        urlopen.assert_not_called()
        self.assertEqual(json.loads(out.getvalue())['name'], 'triage')

    def test_missing_arguments_stop_before_any_call(self):
        with mock.patch.dict('os.environ', {}, clear=True), mock.patch('urllib.request.urlopen') as urlopen:
            with self.assertRaises(SystemExit):
                rc.main(['--application-id', 'a', '--guild-id', 'g'])
        urlopen.assert_not_called()


if __name__ == '__main__':
    unittest.main()
