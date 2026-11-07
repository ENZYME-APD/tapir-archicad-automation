"""Checks the shared dryRun option of the commands that delete or change (#2195), without Archicad.

A command with the shared dry run takes the optional boolean input "dryRun" (#/DryRun in
CommonSchemaDefinitions.json), and its response gets "dryRun": true (#/DryRunFlag) on a dry run,
and, for each target that passed the checks, what a real run would do ("wouldDelete", or
"wouldChange" with the old and new values for the bulk setters in WOULD_CHANGE_COMMANDS). This test
reads the registered commands from the sources the way tools/generate_addon_docs.py does and checks:

  * the shared definitions exist and have the documented shape,
  * every command in DRY_RUN_COMMANDS takes dryRun as #/DryRun, optional, and documents the
    dryRun field of its response as #/DryRunFlag, optional,
  * no other command has a dryRun input, except the ones in COMMANDS_WITH_OWN_DRY_RUN, which had
    one before the shared option and move onto it in the later parts of #2195,
  * the responses a real run and a dry run of DeleteElements give, and its inputs with and
    without dryRun, match its schemas, and the wrong shapes do not.

Standard library only. Run: python archicad-addon/Test/test_dry_run.py
"""

import json
import os
import re
import sys
import unittest

TEST_DIR = os.path.dirname (os.path.abspath (__file__))
REPO_ROOT = os.path.dirname (os.path.dirname (TEST_DIR))
sys.path.insert (0, os.path.join (REPO_ROOT, 'tools'))
import generate_addon_docs  # noqa: E402

# The commands that use the shared dry run. Each later part of #2195 adds its commands here.
DRY_RUN_COMMANDS = {
    'DeleteElements',
    # Part 2: element deletes and removals
    'DeleteSelectedElements',
    'DeleteUserDataOfElements',
    'DeleteElementSets',
    'DeleteIssue',
    'RemoveSolidElementLinks',
    'RemoveElementTrims',
    'RemoveIFCPropertiesOfElements',
    'RemoveIFCClassificationReferencesOfElements',
    # Part 3: attribute, property and classification deletes
    'DeleteAttributes',
    'DeleteAndReplaceAttributes',
    'DeleteAttributeFolders',
    'DeleteUserDataOfAttributes',
    'DeletePropertyGroups',
    'DeletePropertyDefinitions',
    'RemovePropertyFromAttributes',
    'DeleteClassificationSystems',
    'DeleteClassificationItems',
    # Part 6: bulk setters
    'SetPropertyValuesOfElements',
    'SetClassificationsOfElements',
    'SetCategoryValuesOfElements',
    'SetGDLParametersOfElements',
}

# The bulk setters (part 6 of #2195): their dry run gives wouldChange instead of wouldDelete.
WOULD_CHANGE_COMMANDS = {
    'SetPropertyValuesOfElements',
    'SetClassificationsOfElements',
    'SetCategoryValuesOfElements',
    'SetGDLParametersOfElements',
}

# The commands of DRY_RUN_COMMANDS that answer one wouldDelete execution result per target in
# executionResults (the bulk setters of WOULD_CHANGE_COMMANDS answer wouldChange instead).
PER_TARGET_DRY_RUN_COMMANDS = DRY_RUN_COMMANDS - WOULD_CHANGE_COMMANDS - {'DeleteSelectedElements', 'DeleteIssue', 'RemovePropertyFromAttributes', 'DeleteAndReplaceAttributes'}

# The commands that had a dryRun input of their own before #2195. The ones that delete or change
# move onto the shared option (and into DRY_RUN_COMMANDS) in the later parts of #2195.
COMMANDS_WITH_OWN_DRY_RUN = {
    'DeleteNavigatorItems',
    'ImportAttributesFromProject',
    'ReindexAttributes',
    'ReindexLayers',
}

GUID = '0F1E2D3C-4B5A-6978-8796-A5B4C3D2E1F0'


class SchemaError (Exception):
    pass


class Validator:
    """A small JSON schema validator for the keywords the Tapir schemas use: $ref (#/<Name>),
    type, enum, properties, required, additionalProperties, items, minItems, maxItems, oneOf,
    anyOf, allOf and pattern. Other keywords (description, format, ...) are ignored."""

    TYPES = {
        'object': lambda v: isinstance (v, dict),
        'array': lambda v: isinstance (v, list),
        'string': lambda v: isinstance (v, str),
        'boolean': lambda v: isinstance (v, bool),
        'integer': lambda v: isinstance (v, int) and not isinstance (v, bool),
        'number': lambda v: isinstance (v, (int, float)) and not isinstance (v, bool),
        'null': lambda v: v is None,
    }

    def __init__ (self, definitions):
        self.definitions = definitions

    def IsValid (self, value, schema):
        try:
            self.Check (value, schema, '$')
            return True
        except SchemaError:
            return False

    def Check (self, value, schema, path):
        if '$ref' in schema:
            name = schema['$ref']
            if not name.startswith ('#/') or name[2:] not in self.definitions:
                raise AssertionError ('Unknown schema reference ' + name)
            self.Check (value, self.definitions[name[2:]], path)
        if 'type' in schema:
            types = schema['type'] if isinstance (schema['type'], list) else [schema['type']]
            if not any (self.TYPES[t] (value) for t in types):
                raise SchemaError (path + ': not ' + '/'.join (types))
        if 'enum' in schema and value not in schema['enum']:
            raise SchemaError (path + ': not one of ' + repr (schema['enum']))
        if 'pattern' in schema and isinstance (value, str) and re.search (schema['pattern'], value) is None:
            raise SchemaError (path + ': does not match ' + schema['pattern'])
        if isinstance (value, dict):
            properties = schema.get ('properties', {})
            for key in schema.get ('required', []):
                if key not in value:
                    raise SchemaError (path + ': missing ' + key)
            for key, item in value.items ():
                if key in properties:
                    self.Check (item, properties[key], path + '.' + key)
                elif schema.get ('additionalProperties', True) is False:
                    raise SchemaError (path + ': unexpected ' + key)
        if isinstance (value, list):
            if len (value) < schema.get ('minItems', 0) or len (value) > schema.get ('maxItems', len (value)):
                raise SchemaError (path + ': wrong number of items')
            if 'items' in schema:
                for index, item in enumerate (value):
                    self.Check (item, schema['items'], '%s[%d]' % (path, index))
        if 'oneOf' in schema:
            matches = sum (1 for sub in schema['oneOf'] if self.IsValid (value, sub))
            if matches != 1:
                raise SchemaError (path + ': matches %d of oneOf' % matches)
        if 'anyOf' in schema and not any (self.IsValid (value, sub) for sub in schema['anyOf']):
            raise SchemaError (path + ': matches none of anyOf')
        for sub in schema.get ('allOf', []):
            self.Check (value, sub, path)


def LoadCommands ():
    sources = generate_addon_docs.Sources (generate_addon_docs.GetDocsArchicadVersion ())
    interpreter = generate_addon_docs.Interpreter (sources)
    commands = {}
    for _groupName, groupCommands in generate_addon_docs.CollectCommandGroups (sources, interpreter):
        for command in groupCommands:
            commands[command['name']] = {
                'description': command['description'],
                'input': None if command['inputScheme'] is None else json.loads (command['inputScheme']),
                'output': None if command['outputScheme'] is None else json.loads (command['outputScheme']),
            }
    return commands


class DryRunTest (unittest.TestCase):

    @classmethod
    def setUpClass (cls):
        with open (generate_addon_docs.COMMON_SCHEMA_FILE, encoding='utf-8') as f:
            cls.definitions = json.load (f)
        cls.validator = Validator (cls.definitions)
        cls.commands = LoadCommands ()

    def test_shared_definitions (self):
        self.assertEqual (self.definitions['DryRun']['type'], 'boolean')
        self.assertEqual (self.definitions['DryRunFlag']['enum'], [True])
        wouldDelete = self.definitions['WouldDeleteExecutionResult']
        self.assertEqual (sorted (wouldDelete['required']), ['success', 'wouldDelete'])
        self.assertIs (wouldDelete['additionalProperties'], False)
        self.assertEqual (self.definitions['DeletionExecutionResults']['items'], {'$ref': '#/DeletionExecutionResult'})

    def test_dry_run_commands_use_the_shared_option (self):
        for name in sorted (DRY_RUN_COMMANDS):
            with self.subTest (command=name):
                self.assertIn (name, self.commands, 'not a registered command')
                inputSchema = self.commands[name]['input']
                outputSchema = self.commands[name]['output']
                self.assertEqual (inputSchema['properties'].get ('dryRun'), {'$ref': '#/DryRun'})
                self.assertNotIn ('dryRun', inputSchema.get ('required', []))
                self.assertEqual (outputSchema['properties'].get ('dryRun'), {'$ref': '#/DryRunFlag'})
                self.assertNotIn ('dryRun', outputSchema.get ('required', []))
                self.assertIn ('dryRun', self.commands[name]['description'])

    def test_no_other_command_has_a_dry_run (self):
        withDryRun = set ()
        for name, command in self.commands.items ():
            inputSchema = command['input']
            if isinstance (inputSchema, dict) and 'dryRun' in inputSchema.get ('properties', {}):
                withDryRun.add (name)
        self.assertEqual (sorted (withDryRun - DRY_RUN_COMMANDS - COMMANDS_WITH_OWN_DRY_RUN), [],
            'commands with a dryRun input that is not the shared one: add them to DRY_RUN_COMMANDS')
        self.assertEqual (sorted (DRY_RUN_COMMANDS & COMMANDS_WITH_OWN_DRY_RUN), [],
            'a command that moved onto the shared dry run must leave COMMANDS_WITH_OWN_DRY_RUN')

    def test_delete_elements_inputs (self):
        inputSchema = self.commands['DeleteElements']['input']
        elements = [{'elementId': {'guid': GUID}}]
        self.assertTrue (self.validator.IsValid ({'elements': elements}, inputSchema))
        self.assertTrue (self.validator.IsValid ({'elements': elements, 'dryRun': True}, inputSchema))
        self.assertTrue (self.validator.IsValid ({'elements': elements, 'dryRun': False}, inputSchema))
        self.assertFalse (self.validator.IsValid ({'elements': elements, 'dryRun': 'yes'}, inputSchema))
        self.assertFalse (self.validator.IsValid ({'elements': elements, 'dryrun': True}, inputSchema))

    def test_delete_elements_responses (self):
        outputSchema = self.commands['DeleteElements']['output']
        failed = {'success': False, 'error': {'code': -2130313112, 'message': 'The element was not deleted, because it does not exist.'}}
        realRun = {'executionResults': [{'success': True}, failed]}
        dryRun = {'dryRun': True, 'executionResults': [{'success': True, 'wouldDelete': True}, failed]}
        self.assertTrue (self.validator.IsValid (realRun, outputSchema))
        self.assertTrue (self.validator.IsValid (dryRun, outputSchema))
        # A real run never says dryRun: false, and a target is never both deleted and to be deleted.
        self.assertFalse (self.validator.IsValid ({'dryRun': False, 'executionResults': []}, outputSchema))
        self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': True, 'wouldDelete': False}]}, outputSchema))
        self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': False, 'wouldDelete': True}]}, outputSchema))
        self.assertFalse (self.validator.IsValid ({'dryRun': True}, outputSchema))

    def test_would_change_definitions (self):
        wouldChange = self.definitions['WouldChangeExecutionResult']
        self.assertEqual (sorted (wouldChange['required']), ['success', 'wouldChange'])
        self.assertIs (wouldChange['additionalProperties'], False)
        self.assertEqual (sorted (self.definitions['WouldChange']['properties']), ['newValue', 'oldValue'])
        self.assertEqual (self.definitions['ChangeExecutionResults']['items'], {'$ref': '#/ChangeExecutionResult'})

    def test_would_change_commands_report_change_results (self):
        for name in sorted (WOULD_CHANGE_COMMANDS):
            with self.subTest (command=name):
                outputSchema = self.commands[name]['output']
                self.assertEqual (outputSchema['properties']['executionResults']['$ref'], '#/ChangeExecutionResults')
                self.assertIn ('wouldChange', outputSchema['properties']['executionResults']['description'])
                self.assertIn ('wouldChange', self.commands[name]['description'])

    def test_would_change_responses (self):
        failed = {'success': False, 'error': {'code': -2130313112, 'message': 'Failed to set property value for element'}}
        examples = {
            'SetPropertyValuesOfElements': {'oldValue': 'W-001', 'newValue': 'W-002'},
            'SetClassificationsOfElements': {
                'oldValue': {'classificationSystemId': {'guid': GUID}},
                'newValue': {'classificationSystemId': {'guid': GUID}, 'classificationItemId': {'guid': GUID}},
            },
            'SetCategoryValuesOfElements': {
                'oldValue': {'type': 'Position', 'categoryId': {'guid': GUID}},
                'newValue': {'type': 'Position', 'categoryId': {'guid': GUID}, 'categoryValueId': {'guid': GUID}, 'name': 'Exterior'},
            },
            'SetGDLParametersOfElements': {
                'oldValue': [{'name': 'A', 'index': 1, 'type': 'Length', 'value': 1.0}],
                'newValue': [{'name': 'A', 'index': 1, 'type': 'Length', 'value': 1.2}],
            },
        }
        for name, example in sorted (examples.items ()):
            with self.subTest (command=name):
                outputSchema = self.commands[name]['output']
                realRun = {'executionResults': [{'success': True}, failed]}
                dryRun = {'dryRun': True, 'executionResults': [{'success': True, 'wouldChange': example}, failed]}
                partial = {'dryRun': True, 'executionResults': [{'success': True, 'wouldChange': {'oldValue': example['oldValue']}}]}
                self.assertTrue (self.validator.IsValid (realRun, outputSchema))
                self.assertTrue (self.validator.IsValid (dryRun, outputSchema))
                self.assertTrue (self.validator.IsValid (partial, outputSchema))
                # wouldChange has only oldValue and newValue, a failed target never has one, and a
                # changed target is never also to be deleted.
                self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': True, 'wouldChange': {'value': 1}}]}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': False, 'wouldChange': example}]}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': True, 'wouldDelete': True}]}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': False, 'executionResults': []}, outputSchema))

    def test_would_change_inputs (self):
        elementId = {'guid': GUID}
        inputs = {
            'SetPropertyValuesOfElements': {'elementPropertyValues': [{'elementId': elementId, 'propertyId': {'guid': GUID}, 'propertyValue': {'value': 'x'}}]},
            'SetClassificationsOfElements': {'elementClassifications': [{'elementId': elementId, 'classificationId': {'classificationSystemId': {'guid': GUID}}}]},
            'SetCategoryValuesOfElements': {'elementCategoryValues': [{'elementId': elementId, 'type': 'Position', 'categoryValueId': {'guid': GUID}}]},
            'SetGDLParametersOfElements': {'elementsWithGDLParameters': [{'elementId': elementId, 'gdlParameters': [{'name': 'A', 'value': 1.0}]}]},
        }
        for name, parameters in sorted (inputs.items ()):
            with self.subTest (command=name):
                inputSchema = self.commands[name]['input']
                self.assertTrue (self.validator.IsValid (parameters, inputSchema))
                self.assertTrue (self.validator.IsValid (dict (parameters, dryRun=True), inputSchema))
                self.assertFalse (self.validator.IsValid (dict (parameters, dryRun='yes'), inputSchema))

    def test_per_target_responses (self):
        failed = {'success': False, 'error': {'code': -2130313112, 'message': 'Failed.'}}
        for name in sorted (PER_TARGET_DRY_RUN_COMMANDS):
            with self.subTest (command=name):
                outputSchema = self.commands[name]['output']
                self.assertTrue (self.validator.IsValid ({'executionResults': [{'success': True}, failed]}, outputSchema))
                self.assertTrue (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': True, 'wouldDelete': True}, failed]}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': False, 'executionResults': []}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': True, 'executionResults': [{'success': False, 'wouldDelete': True}]}, outputSchema))

    def test_single_result_responses (self):
        failed = {'success': False, 'error': {'code': -2130313112, 'message': 'Failed.'}}
        for name in ['DeleteIssue', 'DeleteSelectedElements', 'RemovePropertyFromAttributes']:
            with self.subTest (command=name):
                outputSchema = self.commands[name]['output']
                self.assertTrue (self.validator.IsValid ({'success': True}, outputSchema))
                self.assertTrue (self.validator.IsValid (failed, outputSchema))
                self.assertTrue (self.validator.IsValid (dict (failed, dryRun=True), outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': False, 'success': True}, outputSchema))
                self.assertFalse (self.validator.IsValid ({'dryRun': True}, outputSchema))
        self.assertTrue (self.validator.IsValid ({'dryRun': True, 'success': True, 'wouldDelete': True}, self.commands['DeleteIssue']['output']))
        self.assertTrue (self.validator.IsValid ({'dryRun': True, 'success': True, 'wouldDelete': True}, self.commands['RemovePropertyFromAttributes']['output']))
        selected = {'dryRun': True, 'success': True, 'wouldDelete': True, 'elements': [{'elementId': {'guid': GUID}}]}
        self.assertTrue (self.validator.IsValid (selected, self.commands['DeleteSelectedElements']['output']))
        self.assertTrue (self.validator.IsValid ({'dryRun': True, 'success': True, 'wouldDelete': False, 'elements': []}, self.commands['DeleteSelectedElements']['output']))
        # DeleteSelectedElements takes no input but dryRun.
        inputSchema = self.commands['DeleteSelectedElements']['input']
        self.assertTrue (self.validator.IsValid ({}, inputSchema))
        self.assertTrue (self.validator.IsValid ({'dryRun': True}, inputSchema))
        self.assertFalse (self.validator.IsValid ({'elements': []}, inputSchema))

    def test_delete_and_replace_attributes_responses (self):
        outputSchema = self.commands['DeleteAndReplaceAttributes']['output']
        replaced = {
            'attributeId': {'guid': GUID},
            'replacementAttributeId': {'guid': GUID},
            'elements': [],
            'hotlinkElements': [],
            'toolDefaults': [],
            'favorites': [],
            'attributes': [],
            'overrideRules': [],
        }
        notFound = {'error': {'code': -2130313112, 'message': 'The Fill to delete was not found.'}}
        self.assertTrue (self.validator.IsValid ({'attributes': [replaced, notFound]}, outputSchema))
        self.assertTrue (self.validator.IsValid ({'dryRun': True, 'attributes': [dict (replaced, wouldDelete=True), notFound]}, outputSchema))
        self.assertFalse (self.validator.IsValid ({'dryRun': False, 'attributes': []}, outputSchema))
        self.assertFalse (self.validator.IsValid ({'dryRun': True, 'attributes': [dict (replaced, wouldDelete=False)]}, outputSchema))
        inputSchema = self.commands['DeleteAndReplaceAttributes']['input']
        attributes = [{'attributeId': {'guid': GUID}, 'replacementAttributeId': {'guid': GUID}}]
        self.assertTrue (self.validator.IsValid ({'attributeType': 'Fill', 'attributes': attributes, 'dryRun': True}, inputSchema))
        self.assertFalse (self.validator.IsValid ({'attributeType': 'Fill', 'attributes': attributes, 'dryRun': 'yes'}, inputSchema))


if __name__ == '__main__':
    unittest.main ()
