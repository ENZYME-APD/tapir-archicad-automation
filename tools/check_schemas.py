"""Checks that every command's schemas are named definitions (CLAUDE.md, *Schemas*).

A command's input schema and response schema must each be only a reference to a definition
in archicad-addon/Sources/RFIX/Images/CommonSchemaDefinitions.json, {"$ref": "#/<Name>"}
(by convention <Command>Parameters and <Command>Response), or the command takes no input.
Every $ref must name a definition that exists.

The schemas that are still written inline are listed in tools/inline_schemas_baseline.json,
which only shrinks: a command that is not in it must not have an inline schema, and a listed
schema that is now a $ref must be taken out of the list (this check says which).

It reads docs/archicad-addon/command_definitions.js and common_schema_definitions.js, so run
python tools/generate_addon_docs.py first (the add-on build check does). Standard library only.

    python tools/check_schemas.py                    check
    python tools/check_schemas.py --write-baseline   list every inline schema of today in the baseline
"""

import argparse
import json
import os
import sys

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
DOCS_DIR = os.path.join (ROOT, 'docs', 'archicad-addon')
BASELINE = os.path.join (ROOT, 'tools', 'inline_schemas_baseline.json')


def ReadJsVariable (path):
    with open (path, encoding='utf-8') as file:
        text = file.read ().strip ()
    return json.loads (text[text.index ('=') + 1:].rstrip (';'))


def IsReference (schema):
    return isinstance (schema, dict) and set (schema) == {'$ref'}


def References (node):
    if isinstance (node, dict):
        if isinstance (node.get ('$ref'), str):
            yield node['$ref']
        for value in node.values ():
            yield from References (value)
    elif isinstance (node, list):
        for item in node:
            yield from References (item)


def Inspect ():
    """(inline schemas as ['<Command> input', '<Command> response'], unresolved references)."""
    groups = ReadJsVariable (os.path.join (DOCS_DIR, 'command_definitions.js'))
    definitions = ReadJsVariable (os.path.join (DOCS_DIR, 'common_schema_definitions.js'))
    inline, unresolved = [], []
    for group in groups:
        for command in group['commands']:
            for key, part in (('inputScheme', 'input'), ('outputScheme', 'response')):
                schema = command.get (key)
                if schema is not None and not IsReference (schema):
                    inline.append ('{} {}'.format (command['name'], part))
                for ref in References (schema):
                    if ref[2:] not in definitions:
                        unresolved.append ('{} {}: {}'.format (command['name'], part, ref))
    for name, definition in definitions.items ():
        for ref in References (definition):
            if ref[2:] not in definitions:
                unresolved.append ('definition {}: {}'.format (name, ref))
    return sorted (inline), unresolved


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--write-baseline', action='store_true', help="write today's inline schemas to the baseline")
    args = parser.parse_args ()

    inline, unresolved = Inspect ()
    if args.write_baseline:
        with open (BASELINE, 'w', encoding='utf-8', newline='\n') as file:
            json.dump ({'_comment': 'Command schemas still written inline; only ever remove lines (tools/check_schemas.py).',
                        'inline': inline}, file, indent=1)
            file.write ('\n')
        print ('{} inline schemas written to {}'.format (len (inline), os.path.relpath (BASELINE, ROOT)))
        return 0

    with open (BASELINE, encoding='utf-8') as file:
        baseline = set (json.load (file)['inline'])
    new = [entry for entry in inline if entry not in baseline]
    done = sorted (baseline - set (inline))
    problems = 0
    for entry in new:
        print ('inline schema: {} - make it a definition in CommonSchemaDefinitions.json and refer to it'
               ' with {{"$ref": "#/<Name>"}} (CLAUDE.md, Schemas)'.format (entry))
        problems += 1
    for entry in done:
        print ('no longer inline: {} - remove it from tools/inline_schemas_baseline.json'.format (entry))
        problems += 1
    for entry in unresolved:
        print ('unresolved $ref: ' + entry)
        problems += 1
    if problems:
        print ('{} problem(s).'.format (problems))
        return 1
    print ('Every command schema is a named definition, apart from the {} listed in the baseline.'.format (len (inline)))
    return 0


if __name__ == '__main__':
    sys.exit (main ())
