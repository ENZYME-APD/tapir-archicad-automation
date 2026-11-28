"""Tests of tools/expand_schemas.py; no Archicad and no repository files needed.

Its output is what a schema refactor is compared against ("the expanded output before and after must
be the same"), so a mistake here shows up as a refactor that looks clean when it changed a schema.

    python -m unittest discover tools/tests
"""

import contextlib
import io
import json
import os
import pathlib
import sys
import tempfile
import unittest

TOOLS = pathlib.Path (__file__).resolve ().parent.parent
sys.path.insert (0, str (TOOLS))

import expand_schemas  # noqa: E402

DEFINITIONS = {
    'Guid': {'type': 'string'},
    'Coordinate': {
        'type': 'object',
        'properties': {'x': {'type': 'number'}, 'y': {'type': 'number'}},
        'required': ['x', 'y'],
    },
    'Node': {
        'type': 'object',
        'properties': {'at': {'$ref': '#/Coordinate'}, 'next': {'$ref': '#/Node'}},
    },
}


class ExpandTest (unittest.TestCase):

    def expand (self, node, ignoreDescriptions = False):
        return expand_schemas.Expand (node, DEFINITIONS, [], ignoreDescriptions)

    def test_a_ref_is_replaced_by_its_definition (self):
        self.assertEqual (self.expand ({'$ref': '#/Coordinate'}),
                          {'type': 'object', 'properties': {'x': {'type': 'number'}, 'y': {'type': 'number'}},
                           'required': ['x', 'y']})

    def test_a_key_next_to_a_ref_is_applied_on_top (self):
        expanded = self.expand ({'$ref': '#/Coordinate', 'description': 'where it is'})
        self.assertEqual (expanded['description'], 'where it is')
        self.assertIn ('x', expanded['properties'])

    def test_a_recursive_definition_keeps_a_marker (self):
        # Stop expanding, the way the renderer does, instead of recursing until the stack ends.
        expanded = self.expand ({'$ref': '#/Node'})
        self.assertEqual (expanded['properties']['next'], {'$recursive': 'Node'})
        self.assertIn ('at', expanded['properties'])

    def test_the_same_definition_twice_is_expanded_twice (self):
        # The stack is carried per branch, not per walk: a sibling ref is not a recursion.
        expanded = self.expand ([{'$ref': '#/Coordinate'}, {'$ref': '#/Coordinate'}])
        self.assertEqual (expanded[0], expanded[1])
        self.assertIn ('x', expanded[1]['properties'])

    def test_an_unresolved_ref_raises (self):
        with self.assertRaises (KeyError):
            self.expand ({'$ref': '#/NotThere'})

    def test_descriptions_can_be_left_out (self):
        node = {'type': 'object', 'description': 'd', 'properties': {'a': {'title': 't', 'type': 'string'}}}
        self.assertEqual (self.expand (node, ignoreDescriptions = True),
                          {'type': 'object', 'properties': {'a': {'type': 'string'}}})

    def test_none_stays_none (self):
        self.assertIsNone (self.expand (None))


class CompareTest (unittest.TestCase):

    def compare (self, beforeValue, afterValue):
        """Compare two written files and return (exit code, what it printed)."""
        before = self.write ('a.json', beforeValue)
        after = self.write ('b.json', afterValue)
        out = io.StringIO ()
        with contextlib.redirect_stdout (out):
            code = expand_schemas.Compare (before, after)
        return code, out.getvalue ()

    def setUp (self):
        self.directory = tempfile.TemporaryDirectory ()
        self.path = pathlib.Path (self.directory.name)

    def tearDown (self):
        self.directory.cleanup ()

    def write (self, name, value):
        path = self.path / name
        path.write_text (json.dumps (value), encoding = 'utf-8')
        return str (path)

    def test_the_same_schemas_compare_equal (self):
        value = {'CreateWalls': {'input': {'a': 1}, 'output': {'b': 2}}}
        code, out = self.compare (value, value)
        self.assertEqual (code, 0)
        self.assertIn ('All expanded schemas are the same.', out)

    def test_a_changed_schema_is_reported (self):
        code, out = self.compare ({'CreateWalls': {'input': {'a': 1}, 'output': {}}},
                                  {'CreateWalls': {'input': {'a': 2}, 'output': {}}})
        self.assertEqual (code, 1)
        self.assertIn ('CreateWalls: input schema differs', out)

    def test_an_unchanged_output_is_not_reported (self):
        code, out = self.compare ({'CreateWalls': {'input': {'a': 1}, 'output': {}}},
                                  {'CreateWalls': {'input': {'a': 1}, 'output': {}}})
        self.assertEqual (code, 0)
        self.assertNotIn ('differs', out)

    def test_a_new_and_a_removed_command_are_reported (self):
        code, out = self.compare ({'Old': {'input': {}, 'output': {}}}, {'New': {'input': {}, 'output': {}}})
        self.assertEqual (code, 1)
        self.assertIn ('New: new command', out)
        self.assertIn ('Old: removed', out)


class ReadJsVariableTest (unittest.TestCase):

    def test_it_reads_the_json_of_a_generated_file (self):
        # generate_addon_docs.py writes 'var gCommands = <JSON>;'.
        with tempfile.TemporaryDirectory () as directory:
            path = pathlib.Path (directory) / 'definitions.js'
            path.write_text ('var gSchemaDefinitions = {"A": {"type": "string"}};', encoding = 'utf-8')
            self.assertEqual (expand_schemas.ReadJsVariable (str (path)), {'A': {'type': 'string'}})


if __name__ == '__main__':
    unittest.main ()
