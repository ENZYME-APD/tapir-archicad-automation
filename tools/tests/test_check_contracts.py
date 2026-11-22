"""Tests of the pure parts of tools/check_contracts.py; no Archicad and no repository files needed.

The validator runs on every pull request, so a change that quietly weakens it would turn a wrong
check green. These pin the rules: what is an error, what is only a note, and which steps are
negative tests whose parameters are out of contract on purpose.

    python -m unittest discover tools/tests
"""

import os
import sys
import unittest

sys.path.insert (0, os.path.dirname (os.path.dirname (os.path.abspath (__file__))))

import check_contracts as cc  # noqa: E402

DEFINITIONS = {
    'Guid': {'type': 'string'},
    'AttributeId': {
        'type': 'object',
        'properties': {'guid': {'$ref': '#/Guid'}},
        'additionalProperties': False,
        'required': ['guid'],
    },
}


def Column ():
    """A tiny input schema: one array of objects that take a width and an optional attribute."""
    return {
        'type': 'object',
        'properties': {
            'columnsData': {
                'type': 'array',
                'items': {
                    'type': 'object',
                    'properties': {
                        'width': {'type': 'number'},
                        'buildingMaterialId': {'$ref': '#/AttributeId'},
                        'floorIndex': {'type': 'integer'},
                    },
                    'additionalProperties': False,
                    'required': ['width'],
                },
            },
        },
        'additionalProperties': False,
        'required': ['columnsData'],
    }


def Validate (value, schema = None, sourceNames = frozenset ()):
    errors, notes = [], []
    cc.CheckValue (value, schema if schema is not None else Column (), DEFINITIONS, 'step', errors, notes, sourceNames)
    return errors, notes


class ValueTest (unittest.TestCase):

    def test_a_valid_value_has_no_findings (self):
        errors, notes = Validate ({'columnsData': [{'width': 0.3, 'floorIndex': 0}]})
        self.assertEqual (errors, [])
        self.assertEqual (notes, [])

    def test_a_missing_required_field_is_an_error (self):
        errors, _ = Validate ({'columnsData': [{'floorIndex': 0}]})
        self.assertEqual (len (errors), 1)
        self.assertIn ('width is required', errors[0])

    def test_a_wrong_type_is_an_error (self):
        errors, _ = Validate ({'columnsData': [{'width': 'wide'}]})
        self.assertEqual (len (errors), 1)
        self.assertIn ('is str, the schema says number', errors[0])

    def test_a_boolean_is_not_an_integer (self):
        # bool is a subclass of int in Python, and a schema boolean is not an integer.
        errors, _ = Validate ({'columnsData': [{'width': 1, 'floorIndex': True}]})
        self.assertEqual (len (errors), 1)
        self.assertIn ('floorIndex', errors[0])

    def test_a_whole_number_float_fits_an_integer (self):
        errors, _ = Validate ({'columnsData': [{'width': 1, 'floorIndex': 2.0}]})
        self.assertEqual (errors, [])

    def test_an_unknown_parameter_is_an_error (self):
        errors, _ = Validate ({'columnsData': [{'width': 1, 'widht': 2}]})
        self.assertEqual (len (errors), 1)
        self.assertIn ('widht is not a parameter of the command', errors[0])

    def test_a_name_the_sources_have_is_only_a_note (self):
        # The published reference lags a weekly release: a field the sources have is not an error.
        errors, notes = Validate ({'columnsData': [{'width': 1, 'dryRun': True}]}, sourceNames = {'dryRun'})
        self.assertEqual (errors, [])
        self.assertEqual (len (notes), 1)
        self.assertIn ('not in the published schema yet', notes[0])

    def test_an_enum_value_must_be_one_of_them (self):
        schema = {'enum': ['Left', 'Right']}
        errors, _ = Validate ('Middle', schema)
        self.assertEqual (len (errors), 1)
        self.assertIn ('"Middle" is not one of "Left", "Right"', errors[0])

    def test_a_placeholder_takes_the_saved_value_s_type (self):
        # ${bm} is filled in by the runner from an earlier step, so nothing can be said about it.
        errors, _ = Validate ({'columnsData': [{'width': '${w}'}]})
        self.assertEqual (errors, [])

    def test_a_placeholder_inside_a_string_does_not_hide_the_type (self):
        errors, _ = Validate ({'columnsData': [{'width': '${w} m'}]})
        self.assertEqual (len (errors), 1)

    def test_one_of_reports_the_closest_branch (self):
        schema = {'oneOf': [
            {'type': 'object', 'properties': {'width': {'type': 'number'}}, 'additionalProperties': False, 'required': ['width']},
            {'type': 'object', 'properties': {'profileId': {'type': 'string'}}, 'additionalProperties': False, 'required': ['profileId']},
        ]}
        errors, _ = Validate ({'width': 1, 'profileId': 'x'}, schema)
        self.assertEqual (len (errors), 1)
        self.assertIn ('matches none of the 2 alternatives', errors[0])
        self.assertIn ('profileId is not a parameter of the command', errors[0])
        self.assertNotIn ('matches none of the 2 alternatives (closest: matches none', errors[0])

    def test_a_ref_is_followed_through_all_of (self):
        schema = {'allOf': [{'$ref': '#/AttributeId'}, {'description': 'a building material'}]}
        errors, _ = Validate ({'guid': 'x'}, schema)
        self.assertEqual (errors, [])
        errors, _ = Validate ({'attributeId': {'guid': 'x'}}, schema)
        self.assertEqual (len (errors), 2)          # the wrapper is both missing guid and unknown
        self.assertTrue (any ('guid is required' in e for e in errors))
        self.assertTrue (any ('attributeId is not a parameter' in e for e in errors))


class ResponsePathTest (unittest.TestCase):

    OUTPUT = {
        'type': 'object',
        'properties': {
            'elements': {'type': 'array', 'items': {
                'type': 'object',
                'properties': {'elementId': {'$ref': '#/AttributeId'}},
                'additionalProperties': False,
            }},
            'free': {'type': 'object'},
        },
        'additionalProperties': False,
    }

    def resolve (self, path, sourceNames = frozenset ()):
        return cc.ResolvePath (path, self.OUTPUT, DEFINITIONS, sourceNames)

    def test_a_path_that_exists_resolves (self):
        self.assertEqual (self.resolve ('elements[0].elementId.guid'), (True, None))

    def test_an_envelope_error_path_is_always_accepted (self):
        self.assertEqual (self.resolve ('__envelopeError__.code'), (True, None))

    def test_a_field_the_schema_lacks_and_the_sources_lack_is_an_error (self):
        self.assertEqual (self.resolve ('elements[0].bogus'), (False, None))

    def test_a_field_the_sources_have_is_a_lagging_note (self):
        # This used to return (True, None), so the note could never print.
        self.assertEqual (self.resolve ('elements[0].constructionElementsOnly', {'constructionElementsOnly'}),
                          (True, 'lagging'))

    def test_an_object_without_properties_cannot_be_called_wrong (self):
        self.assertEqual (self.resolve ('free.anything.at.all'), (True, None))


class RefusalTest (unittest.TestCase):
    """A negative test sends parameters the command must refuse; those are out of contract on purpose."""

    def assertRefusal (self, step, expected = True):
        self.assertIs (cc.ExpectsRefusal (step), expected, step)

    def test_allow_envelope_error_is_a_refusal (self):
        self.assertRefusal ({'allowEnvelopeError': True})

    def test_an_envelope_error_expectation_is_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': '__envelopeError__', 'exists': True}]})

    def test_a_top_level_error_is_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': 'error.code', 'eq': 1}]})

    def test_a_per_item_error_is_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': 'attributeIds[0].error', 'exists': True}]})

    def test_a_bare_success_false_is_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': 'success', 'eq': False}]})

    def test_a_per_item_success_false_is_a_refusal (self):
        # executionResults[0].success: false - the shape the first CI run missed.
        self.assertRefusal ({'expect': [{'path': 'executionResults[0].success', 'eq': False}]})

    def test_success_true_is_not_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': 'executionResults[0].success', 'eq': True}]}, expected = False)

    def test_a_plain_expectation_is_not_a_refusal (self):
        self.assertRefusal ({'expect': [{'path': 'elements[0].elementId.guid', 'exists': True}]}, expected = False)


class StepTest (unittest.TestCase):

    COMMANDS = {'CreateColumns': {'input': Column (), 'output': ResponsePathTest.OUTPUT}}

    def check (self, step):
        return cc.CheckStep ('create_columns.json', step, self.COMMANDS, DEFINITIONS, frozenset ())

    def test_a_good_step_passes (self):
        errors, notes = self.check ({
            'id': 'a column', 'command': 'CreateColumns',
            'params': {'columnsData': [{'width': 0.3}]},
            'expect': [{'path': 'elements[0].elementId.guid', 'exists': True}],
            'save': {'c1': 'elements[0].elementId.guid'},
        })
        self.assertEqual ((errors, notes), ([], []))

    def test_a_bad_parameter_and_a_bad_response_path_are_both_reported (self):
        errors, _ = self.check ({
            'id': 'a column', 'command': 'CreateColumns',
            'params': {'columnsData': [{'width': 'wide'}]},
            'expect': [{'path': 'elements[0].bogus', 'exists': True}],
        })
        self.assertEqual (len (errors), 2)
        self.assertTrue (any ('is str' in e for e in errors))
        self.assertTrue (any ('response has no "bogus"' in e for e in errors))

    def test_an_exists_false_path_is_not_resolved (self):
        # mep_system_id_read_write_round_trip.json asserts a field is absent: that path is not expected.
        errors, _ = self.check ({
            'id': 'not wrapped', 'command': 'CreateColumns',
            'expect': [{'path': 'elements[0].mepSystemId.attributeId', 'exists': False}],
        })
        self.assertEqual (errors, [])

    def test_a_refusal_step_is_not_validated (self):
        errors, _ = self.check ({
            'id': 'a bad column is refused', 'command': 'CreateColumns',
            'params': {'columnsData': [{'width': 'not a number'}]},
            'expect': [{'path': 'executionResults[0].success', 'eq': False}],
        })
        self.assertEqual (errors, [])

    def test_an_archicad_json_command_is_skipped (self):
        errors, notes = self.check ({'id': 'raw', 'command': 'API.GetElementIds', 'params': {'anything': 1}})
        self.assertEqual ((errors, notes), ([], []))

    def test_a_command_the_reference_lacks_is_a_note_not_an_error (self):
        errors, notes = self.check ({'id': 'new', 'command': 'CreateRadialDimensions', 'params': {}})
        self.assertEqual (errors, [])
        self.assertIn ('not in the published command reference yet', notes[0])


if __name__ == '__main__':
    unittest.main ()
