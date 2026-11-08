"""Prints every command's input and response schema with the shared definitions expanded.

The docs generator (tools/generate_addon_docs.py) writes the schemas as they are, with
{"$ref": "#/<Name>"} pointing into the shared definitions; only the browser renderer expands
them. This script expands them the same way, so a change that moves an inline schema into a
shared definition (CLAUDE.md, *Schemas*) can be checked: the expanded output before and after
must be the same for every command the change touches.

    python tools/generate_addon_docs.py
    python tools/expand_schemas.py --out before.json        # on main
    ... make the change, run generate_addon_docs.py again ...
    python tools/expand_schemas.py --out after.json
    python tools/expand_schemas.py --compare before.json after.json

It reads docs/archicad-addon/command_definitions.js and common_schema_definitions.js (the
generator's output). Standard library only.
"""

import argparse
import json
import os
import sys

DOCS_DIR = os.path.join (os.path.dirname (os.path.abspath (__file__)), '..', 'docs', 'archicad-addon')


def ReadJsVariable (path):
    """ The generated files are 'var gName = <JSON>;'. """
    with open (path, encoding='utf-8') as file:
        text = file.read ().strip ()
    start = text.index ('=') + 1
    if text.endswith (';'):
        text = text[:-1]
    return json.loads (text[start:])


def Expand (node, definitions, stack, ignoreDescriptions):
    if isinstance (node, list):
        return [Expand (item, definitions, stack, ignoreDescriptions) for item in node]
    if not isinstance (node, dict):
        return node
    if '$ref' in node:
        name = node['$ref'][2:] if node['$ref'].startswith ('#/') else node['$ref']
        if name not in definitions:
            raise KeyError ('unresolved $ref: ' + node['$ref'])
        if name in stack:
            # A recursive definition: keep the reference itself, as the renderer does.
            return {'$recursive': name}
        expanded = Expand (definitions[name], definitions, stack + [name], ignoreDescriptions)
        # Keys next to the $ref (e.g. a description) apply on top of the definition.
        for key, value in node.items ():
            if key != '$ref':
                expanded = dict (expanded)
                expanded[key] = Expand (value, definitions, stack, ignoreDescriptions)
        return expanded
    result = {}
    for key, value in node.items ():
        if ignoreDescriptions and key in ('description', 'title'):
            continue
        result[key] = Expand (value, definitions, stack, ignoreDescriptions)
    return result


def ExpandAll (docsDir, ignoreDescriptions):
    groups = ReadJsVariable (os.path.join (docsDir, 'command_definitions.js'))
    definitions = ReadJsVariable (os.path.join (docsDir, 'common_schema_definitions.js'))
    result = {}
    for group in groups:
        for command in group['commands']:
            result[command['name']] = {
                'input': Expand (command.get ('inputScheme'), definitions, [], ignoreDescriptions),
                'output': Expand (command.get ('outputScheme'), definitions, [], ignoreDescriptions),
            }
    return result


def Compare (beforePath, afterPath):
    with open (beforePath, encoding='utf-8') as file:
        before = json.load (file)
    with open (afterPath, encoding='utf-8') as file:
        after = json.load (file)
    changed = []
    for name in sorted (set (before) | set (after)):
        if name not in before:
            changed.append (name + ': new command')
        elif name not in after:
            changed.append (name + ': removed')
        else:
            for part in ('input', 'output'):
                if before[name][part] != after[name][part]:
                    changed.append (name + ': ' + part + ' schema differs')
    for line in changed:
        print (line)
    print ('{} command(s) differ.'.format (len (changed)) if changed else 'All expanded schemas are the same.')
    return 1 if changed else 0


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--docs-dir', default=DOCS_DIR, help='where generate_addon_docs.py wrote its output')
    parser.add_argument ('--out', help='write the expanded schemas to this file (default: stdout)')
    parser.add_argument ('--ignore-descriptions', action='store_true',
                         help='leave out description and title texts, to compare only the structure')
    parser.add_argument ('--compare', nargs=2, metavar=('BEFORE', 'AFTER'),
                         help='compare two outputs; lists the commands whose expanded schemas differ')
    args = parser.parse_args ()

    if args.compare:
        return Compare (*args.compare)
    expanded = ExpandAll (args.docs_dir, args.ignore_descriptions)
    text = json.dumps (expanded, indent=1, sort_keys=True, ensure_ascii=False)
    if args.out:
        with open (args.out, 'w', encoding='utf-8') as file:
            file.write (text + '\n')
        print ('{} commands written to {}'.format (len (expanded), args.out))
    else:
        print (text)
    return 0


if __name__ == '__main__':
    sys.exit (main ())
