"""Writes tapir_archicad/commands.py: one method per Tapir command, with keyword arguments from its input schema.

It reads the command reference the weekly release keeps up to date
(docs/archicad-addon/command_definitions.js and common_schema_definitions.js), so run it after
tools/generate_addon_docs.py:

    python python-client/generate.py          # write tapir_archicad/commands.py
    python python-client/generate.py --check  # exit 1 when tapir_archicad/commands.py is out of date

Standard library only. The output depends only on the two input files, so the same input always
gives the same file (tests/test_client.py checks the committed file against it).
"""

import argparse
import json
import keyword
import pathlib
import re
import sys

HERE = pathlib.Path (__file__).resolve ().parent
REPO_ROOT = HERE.parent
COMMANDS_JS = REPO_ROOT / 'docs' / 'archicad-addon' / 'command_definitions.js'
DEFINITIONS_JS = REPO_ROOT / 'docs' / 'archicad-addon' / 'common_schema_definitions.js'
OUTPUT = HERE / 'tapir_archicad' / 'commands.py'

# The longest a parameter's description may be in a docstring; the full text is in the command reference.
MAX_PARAMETER_DESCRIPTION = 300
DOCS_URL = 'https://archicad-api.com/archicad-addon/#'


def LoadJsVariable (path):
    """The JSON value of a file of the form `var gName = <JSON>;`."""
    text = path.read_text (encoding = 'utf-8')
    start = text.index ('=') + 1
    return json.loads (text[start:].strip ().rstrip (';'))


def SnakeCase (name):
    """GetAddOnVersion -> get_add_on_version, Get3DProjectionSettings -> get_3d_projection_settings."""
    name = re.sub (r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
    name = re.sub (r'([a-z])([A-Z0-9])', r'\1_\2', name)
    name = re.sub (r'([0-9])([A-Z][a-z])', r'\1_\2', name)
    name = name.replace ('-', '_').lower ()
    if keyword.iskeyword (name) or name in ('self', 'run'):
        name += '_'
    return name


def Resolve (schema, definitions):
    """The schema a {"$ref": "#/Name"} points to (followed until it is no reference), else the schema."""
    seen = set ()
    while isinstance (schema, dict) and '$ref' in schema:
        name = schema['$ref'].split ('/')[-1]
        if name in seen or name not in definitions:
            break
        seen.add (name)
        schema = definitions[name]
    return schema


def Parameters (schema, definitions):
    """The top-level parameters of an input schema: [(jsonName, schema, required)], in schema order.

    The properties of oneOf/anyOf/allOf branches count too, as optional, since which of them is
    needed depends on the branch."""
    found = {}
    order = []

    def Collect (current, requiredHere):
        current = Resolve (current, definitions)
        if not isinstance (current, dict):
            return
        required = set (current.get ('required', [])) if requiredHere else set ()
        for name, propertySchema in current.get ('properties', {}).items ():
            if name not in found:
                order.append (name)
                found[name] = (propertySchema, name in required)
        for key in ('oneOf', 'anyOf', 'allOf'):
            for branch in current.get (key, []):
                Collect (branch, key == 'allOf' and requiredHere)

    Collect (schema, True)
    return [(name, found[name][0], found[name][1]) for name in order]


def TypeText (schema, definitions):
    """A short type for a docstring: 'string', 'array of Elements', 'ElementIdArrayItem', ..."""
    if not isinstance (schema, dict):
        return 'any'
    if '$ref' in schema:
        return schema['$ref'].split ('/')[-1]
    for key in ('oneOf', 'anyOf'):
        if key in schema:
            return ' or '.join (dict.fromkeys (TypeText (branch, definitions) for branch in schema[key]))
    schemaType = schema.get ('type')
    if isinstance (schemaType, list):
        return ' or '.join (schemaType)
    if schemaType == 'array':
        return 'array of ' + TypeText (schema.get ('items'), definitions)
    if 'enum' in schema:
        values = ', '.join (json.dumps (value) for value in schema['enum'])
        return '{} ({})'.format (schemaType or 'value', values)
    return schemaType or 'any'


def DescriptionOf (schema, definitions):
    if not isinstance (schema, dict):
        return ''
    description = schema.get ('description')
    if not description:
        description = Resolve (schema, definitions).get ('description', '') if '$ref' in schema else ''
    description = ' '.join (str (description).split ())
    if len (description) > MAX_PARAMETER_DESCRIPTION:
        description = description[:MAX_PARAMETER_DESCRIPTION - 3].rstrip () + '...'
    return description


def DocstringText (text):
    """Text that is safe inside a triple-quoted docstring."""
    return text.replace ('\\', '\\\\').replace ('"""', '\\"\\"\\"')


def Wrap (text, width, indent):
    lines = []
    line = ''
    for word in text.split ():
        if line and len (line) + 1 + len (word) > width:
            lines.append (line)
            line = word
        else:
            line = word if not line else line + ' ' + word
    if line:
        lines.append (line)
    return [indent + l for l in lines]


def RenderCommand (command, definitions):
    name = command['name']
    method = SnakeCase (name)
    parameters = Parameters (command.get ('inputScheme'), definitions)

    arguments = []
    used = {method}
    for jsonName, schema, required in parameters:
        argument = SnakeCase (jsonName)
        if argument in used:
            raise ValueError ('{}: parameter {} gives the argument name {} twice'.format (name, jsonName, argument))
        used.add (argument)
        arguments.append ((argument, jsonName, schema, required))
    # required arguments first: a keyword-only argument without default may follow one with a default,
    # but keeping them first reads better in help ()
    arguments.sort (key = lambda item: not item[3])

    signature = ['self']
    if arguments:
        signature.append ('*')
        signature += [argument if required else argument + ' = None' for argument, _, _, required in arguments]

    lines = ['    def {} ({}):'.format (method, ', '.join (signature))]
    doc = Wrap (DocstringText (command.get ('description', '')), 96, '        ')
    doc.append ('')
    doc.append ('        Tapir command {} (Tapir {}): {}{}'.format (name, command.get ('version', '?'), DOCS_URL, name))
    if arguments:
        doc.append ('')
        doc.append ('        Arguments:')
        for argument, jsonName, schema, required in arguments:
            head = '{} ({}{}, {})'.format (argument, jsonName + ', ' if jsonName != argument else '', TypeText (schema, definitions), 'required' if required else 'optional')
            description = DescriptionOf (schema, definitions)
            text = head + (': ' + description if description else '')
            wrapped = Wrap (DocstringText (text), 96, '            ')
            wrapped[0] = '        ' + wrapped[0].lstrip ()
            doc += wrapped
    lines.append ('        """' + doc[0].lstrip ())
    lines += [l if l.strip () else '' for l in doc[1:]]
    lines.append ('        """')

    if arguments:
        pairs = ', '.join ('({!r}, {})'.format (jsonName, argument) for argument, jsonName, _, _ in arguments)
        lines.append ('        return self.run ({!r}, _Parameters ({}))'.format (name, pairs))
    else:
        lines.append ('        return self.run ({!r}, {{}})'.format (name))
    return method, lines


def Render (commandGroups, definitions):
    lines = [
        '# Generated by python-client/generate.py from docs/archicad-addon/command_definitions.js',
        '# and common_schema_definitions.js. Do not edit: run `python python-client/generate.py`.',
        '"""One method per Tapir command; Client (tapir_archicad.client) runs them."""',
        '',
        '',
        'def _Parameters (*pairs):',
        '    """The JSON parameters of a command: the (jsonName, value) pairs whose value is not None."""',
        '    return {name: value for name, value in pairs if value is not None}',
        '',
        '',
        'class Commands:',
        '    """The Tapir commands, grouped as in the command reference. Each returns the command\'s',
        '    response (a dict); a failed command raises tapir_archicad.TapirError."""',
        '',
        '    def run (self, command, parameters):',
        '        raise NotImplementedError',
    ]
    methods = set ()
    for group in commandGroups:
        lines.append ('')
        lines.append ('    # {}'.format (group['name']))
        for command in group['commands']:
            method, commandLines = RenderCommand (command, definitions)
            if method in methods:
                raise ValueError ('two commands give the method name ' + method)
            methods.add (method)
            lines.append ('')
            lines += commandLines
    return '\n'.join (lines) + '\n'


def RenderFromFiles ():
    return Render (LoadJsVariable (COMMANDS_JS), LoadJsVariable (DEFINITIONS_JS))


def Main ():
    parser = argparse.ArgumentParser (description = __doc__.splitlines ()[0])
    parser.add_argument ('--check', action = 'store_true', help = 'exit 1 when tapir_archicad/commands.py is out of date, write nothing')
    args = parser.parse_args ()
    text = RenderFromFiles ()
    if args.check:
        current = OUTPUT.read_text (encoding = 'utf-8') if OUTPUT.exists () else ''
        if current != text:
            print ('{} is out of date: run python python-client/generate.py'.format (OUTPUT.relative_to (REPO_ROOT)))
            return 1
        print ('{} is up to date'.format (OUTPUT.relative_to (REPO_ROOT)))
        return 0
    with open (OUTPUT, 'w', encoding = 'utf-8', newline = '\n') as file:
        file.write (text)
    print ('wrote {}'.format (OUTPUT.relative_to (REPO_ROOT)))
    return 0


if __name__ == '__main__':
    sys.exit (Main ())
