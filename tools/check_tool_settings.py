"""Checks the Archicad tool settings tables of the Grasshopper plugin.

Each grasshopper-plugin/TapirGrasshopperPlugin/ToolSettings/<Tool>.json lists,
for one Archicad tool, every setting the Add-On reads or writes: its Tapir JSON
keys (GetDetailsOfElements and Create/Modify), the Archicad API member, the
label of its control in the tool's settings dialog, and the dialog panel
(group) and the panel's subheading (subgroup) it is on. The Grasshopper tool
components take their names from these tables (see CLAUDE.md, "Naming from
the Archicad GUI").

The keys are read from the Add-On sources, without Archicad:
  * GetDetailsOfElements: the properties of the tool's details schema (e.g.
    WallDetails) in the common schema definitions, following $ref and allOf,
  * Create/Modify: the item properties of the commands' input schemas,
    evaluated like tools/generate_addon_docs.py does.

Checks, per table:
  * every key of the Add-On is in the table exactly once (a field, or
    plainKeys for keys that are no setting, like elementId), and the table
    has no key the Add-On does not have,
  * labels are unique, case-insensitively (the JSON is translated back to API
    names by label), and every label names its source.

Usage:
  python tools/check_tool_settings.py              check every table
  python tools/check_tool_settings.py --markdown   also print each table as Markdown
"""

import argparse
import glob
import json
import os
import sys

import generate_addon_docs

REPO_ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
TOOL_SETTINGS_DIR = os.path.join (REPO_ROOT, 'grasshopper-plugin', 'TapirGrasshopperPlugin', 'ToolSettings')

FIELD_PROPERTIES = {'get', 'set', 'getFrom', 'apiMember', 'apiDescription', 'label', 'labelSource', 'group', 'subgroup'}
GET_FROM_VALUES = {'typeSpecificDetails', 'element'}


def GetSchemaProperties (definitions, node, seen=()):
    keys = set ()
    if not isinstance (node, dict):
        return keys
    if '$ref' in node:
        name = node['$ref'].split ('/')[-1]
        if name not in definitions:
            raise ValueError ('schema definition %s not found' % name)
        if name not in seen:
            keys |= GetSchemaProperties (definitions, definitions[name], seen + (name,))
    keys |= set (node.get ('properties', {}))
    for combinator in ('allOf', 'oneOf', 'anyOf'):
        for sub in node.get (combinator, []):
            keys |= GetSchemaProperties (definitions, sub, seen)
    return keys


def GetDetailsKeys (getSchema):
    with open (generate_addon_docs.COMMON_SCHEMA_FILE, encoding='utf-8') as f:
        definitions = json.load (f)
    if getSchema not in definitions:
        raise ValueError ('details schema %s not found in the common schema definitions' % getSchema)
    return GetSchemaProperties (definitions, definitions[getSchema])


def GetSchemaItemKeys (interpreter, className):
    schema = json.loads (interpreter.CallMethod (className, 'GetInputParametersSchema'))
    arrays = [p for p in schema['properties'].values () if p.get ('type') == 'array']
    if len (arrays) != 1:
        raise ValueError ('%s: expected one array parameter in the input schema' % className)
    return set (arrays[0]['items'].get ('properties', {}))


def CheckTable (path, interpreter):
    errors = []
    with open (path, encoding='utf-8') as f:
        table = json.load (f)
    name = os.path.basename (path)
    commands = table.get ('commands', {})
    fields = table.get ('fields', [])
    plainKeys = set (table.get ('plainKeys', []))

    getKeys = GetDetailsKeys (commands['getSchema'])
    setKeys = set ()
    for role in ('create', 'modify'):
        if role in commands:
            setKeys |= GetSchemaItemKeys (interpreter, commands[role])

    tableGet = {}
    tableSet = {}
    labels = {}
    for index, field in enumerate (fields):
        where = '%s field %d (%s)' % (name, index + 1, field.get ('get') or field.get ('set'))
        unknown = set (field) - FIELD_PROPERTIES
        if unknown:
            errors.append ('%s: unknown properties %s' % (where, ', '.join (sorted (unknown))))
        if not field.get ('get') and not field.get ('set'):
            errors.append ('%s: needs a get or a set key' % where)
        if field.get ('getFrom', 'typeSpecificDetails') not in GET_FROM_VALUES:
            errors.append ('%s: getFrom must be one of %s' % (where, ', '.join (sorted (GET_FROM_VALUES))))
        if field.get ('get') and field.get ('getFrom', 'typeSpecificDetails') == 'typeSpecificDetails':
            tableGet.setdefault (field['get'], []).append (where)
        if field.get ('set'):
            tableSet.setdefault (field['set'], []).append (where)
        label = field.get ('label')
        if label:
            if not field.get ('labelSource'):
                errors.append ('%s: label "%s" has no labelSource' % (where, label))
            labels.setdefault (label.lower (), []).append (where)
        elif field.get ('labelSource'):
            errors.append ('%s: labelSource without a label' % where)
        if field.get ('subgroup') and not field.get ('group'):
            errors.append ('%s: subgroup without a group' % where)

    for key, places in sorted (tableGet.items ()):
        if len (places) > 1:
            errors.append ('%s: get key "%s" is in more than one field: %s' % (name, key, '; '.join (places)))
    for key, places in sorted (tableSet.items ()):
        if len (places) > 1:
            errors.append ('%s: set key "%s" is in more than one field: %s' % (name, key, '; '.join (places)))
        if key in plainKeys:
            errors.append ('%s: set key "%s" is both a field and a plain key' % (name, key))
    for label, places in sorted (labels.items ()):
        if len (places) > 1:
            errors.append ('%s: label "%s" is used more than once: %s' % (name, label, '; '.join (places)))

    for key in sorted (getKeys - set (tableGet)):
        errors.append ('%s: GetDetailsOfElements returns "%s", which is not in the table' % (name, key))
    for key in sorted (set (tableGet) - getKeys):
        errors.append ('%s: get key "%s" is not in the details schema %s' % (name, key, commands['getSchema']))
    for key in sorted (setKeys - set (tableSet) - plainKeys):
        errors.append ('%s: the Create/Modify schemas take "%s", which is not in the table' % (name, key))
    for key in sorted ((set (tableSet) | plainKeys) - setKeys):
        errors.append ('%s: set key "%s" is in no Create/Modify schema' % (name, key))
    return table, errors


def FormatMarkdown (table):
    def Cell (value):
        return '' if value is None else str (value).replace ('|', '\\|')

    lines = [
        '### %s (Archicad %s)' % (table['tool'], table.get ('archicadVersion', '')),
        '',
        '| Group | Subgroup | GUI label | Get key | Set key | API member | Label source |',
        '| --- | --- | --- | --- | --- | --- | --- |',
    ]
    for field in table['fields']:
        lines.append ('| %s |' % ' | '.join (Cell (field.get (k)) for k in ('group', 'subgroup', 'label', 'get', 'set', 'apiMember', 'labelSource')))
    labelled = sum (1 for field in table['fields'] if field.get ('label'))
    lines.append ('')
    lines.append ('%d of %d fields have a GUI label.' % (labelled, len (table['fields'])))
    return '\n'.join (lines)


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--markdown', action='store_true', help='also print each table as Markdown')
    args = parser.parse_args ()

    try:
        sources = generate_addon_docs.Sources (generate_addon_docs.GetDocsArchicadVersion ())
        interpreter = generate_addon_docs.Interpreter (sources)
    except generate_addon_docs.GeneratorError as e:
        print ('error: ' + str (e), file=sys.stderr)
        return 2

    paths = sorted (glob.glob (os.path.join (TOOL_SETTINGS_DIR, '*.json')))
    allErrors = []
    for path in paths:
        try:
            table, errors = CheckTable (path, interpreter)
        except (ValueError, KeyError, generate_addon_docs.GeneratorError) as e:
            allErrors.append ('%s: %s' % (os.path.basename (path), e))
            continue
        allErrors.extend (errors)
        if args.markdown:
            print (FormatMarkdown (table))
            print ()

    for error in allErrors:
        print ('error: ' + error, file=sys.stderr)
    if allErrors:
        return 1
    print ('%d tool settings table(s) are consistent with the Add-On.' % len (paths), file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit (main ())
