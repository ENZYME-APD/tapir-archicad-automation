"""Scaffolds new Tapir JSON commands from a short spec file.

For every command in the spec it writes the parts that are the same for every
command, so only the Archicad API calls inside Execute are left to write by hand:

  * the class declaration in Sources/<file>.hpp
  * the constructor, GetName, GetInputParametersSchema, GetRawResponseSchema and
    an Execute skeleton (parameter parsing, per-item loop, error results, undo
    scope, Archicad version guard) in Sources/<file>.cpp
  * the RegisterCommand call in AddOnMain.cpp, with the version from
    tools/package_info.json
  * a starter Python example in archicad-addon/Examples/
  * a starter check in archicad-addon/Test/checks/ for the community test runner
    (tools/community-test-runner), which tests pull requests on real Archicad

Existing commands are never touched: a command whose class already exists is
skipped. Standard library only; a .yaml spec also works when PyYAML is installed.

Usage:
  python tools/new_command.py spec.json            write into the repository
  python tools/new_command.py spec.json --dry-run  print what would be written
  python tools/new_command.py spec.json --schemas  print only the expanded JSON schemas

Spec format (JSON list, or {"commands": [...]}), one object per command:

  {
    "name": "CreateLevelDimensions",       command name, CamelCase
    "file": "ExtendedElementCommands",     Sources/<file>.cpp/.hpp (created if missing)
    "group": "elementCommands",            CommandGroup variable in AddOnMain.cpp
    "description": "Creates level dimensions. Available from Archicad 30.",
    "requires": 30,                        optional: Archicad version guard (ServerMainVers_3000)
    "undoable": true,                      optional: default true unless the name starts with Get
    "commonSchema": true,                  optional: CommonSchema::Used (default) or NotUsed
    "version": "1.7.3",                    optional: defaults to tools/package_info.json
    "example": true,                       optional: write Examples/<snake_case>.py (default true)
    "check": true,                         optional: write Test/checks/<snake_case>.json (default true)
    "input": { ... compact schema ... },   null or omitted for no parameters
    "output": "executionResults" | "elements" | { ... compact schema ... }
  }

Compact schema notation:
  "string" "number" "integer" "boolean"     plain types
  "#Coordinate3D"                           $ref into the common schema definitions
  "enum(Left,Center,Right)"                 string enum
  any of the above + " | text"              adds a description, e.g. "#ElementId | The parent wall."
  {"field": ..., "other?": ...}             object; a trailing ? marks an optional field;
                                            "_desc" gives the object a description
  [item] or [item, "description"]           array of item

Output shortcuts:
  "executionResults"  -> {"executionResults": #ExecutionResults}   (modify / connect / delete)
  "elements"          -> {"elements": #ElementIdsOrErrors}        (create)

When the input is a single required array of objects (inline, or a #Ref such as
#AttributeIdArrayItem) and the output is "executionResults", "elements" or one
array of #...OrError items (e.g. {"attributeIds": ["#AttributeIdOrError"]}),
Execute gets a loop with one result per input item. Fields that are #Refs to a
{guid} id (ElementId, AttributeId, ...) are converted to API_Guid.
"""

import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
SOURCES = os.path.join (ROOT, 'archicad-addon', 'Sources')
EXAMPLES = os.path.join (ROOT, 'archicad-addon', 'Examples')
CHECKS = os.path.join (ROOT, 'archicad-addon', 'Test', 'checks')
ADDON_MAIN = os.path.join (SOURCES, 'AddOnMain.cpp')
PACKAGE_INFO = os.path.join (ROOT, 'tools', 'package_info.json')

PLAIN_TYPES = {'string', 'number', 'integer', 'boolean'}
OUTPUT_SHORTCUTS = {
    'executionResults': {'executionResults': '#ExecutionResults'},
    'elements': {'elements': '#ElementIdsOrErrors'},
}


class SpecError (Exception):
    pass


# ---------------------------------------------------------------- schema --

def split_desc (text):
    if ' | ' in text:
        kind, desc = text.split (' | ', 1)
        return kind.strip (), desc.strip ()
    return text.strip (), None


def expand (node, where):
    """Compact notation -> JSON schema dict, in Tapir's key order."""
    if isinstance (node, str):
        kind, desc = split_desc (node)
        if kind.startswith ('#'):
            out = {'$ref': '#/' + kind[1:]}
        elif kind.startswith ('enum(') and kind.endswith (')'):
            values = [v.strip () for v in kind[5:-1].split (',') if v.strip ()]
            if not values:
                raise SpecError (f'{where}: empty enum')
            out = {'type': 'string'}
            if desc:
                out['description'] = desc
                desc = None
            out['enum'] = values
        elif kind in PLAIN_TYPES:
            out = {'type': kind}
        else:
            raise SpecError (f'{where}: unknown type "{kind}" (use string, number, integer, boolean, #Ref or enum(...))')
        if desc:
            out['description'] = desc
        return out
    if isinstance (node, list):
        if len (node) not in (1, 2) or (len (node) == 2 and not isinstance (node[1], str)):
            raise SpecError (f'{where}: an array is written [item] or [item, "description"]')
        out = {'type': 'array'}
        if len (node) == 2:
            out['description'] = node[1]
        out['items'] = expand (node[0], where + '[]')
        return out
    if isinstance (node, dict):
        out = {'type': 'object'}
        if '_desc' in node:
            out['description'] = node['_desc']
        props, required = {}, []
        for key, value in node.items ():
            if key == '_desc':
                continue
            optional = key.endswith ('?')
            name = key[:-1] if optional else key
            if not re.fullmatch (r'[A-Za-z_][A-Za-z0-9_]*', name):
                raise SpecError (f'{where}: bad field name "{key}"')
            props[name] = expand (value, f'{where}.{name}')
            if not optional:
                required.append (name)
        out['properties'] = props
        out['additionalProperties'] = False
        if required:
            out['required'] = required
        return out
    raise SpecError (f'{where}: cannot read {node!r}')


def schema_literal (schema):
    text = json.dumps (schema, indent=4, ensure_ascii=False)
    if ')"' in text:
        raise SpecError ('a description contains )" which would end the raw string literal')
    lines = text.split ('\n')
    return '\n'.join ([lines[0]] + ['    ' + line for line in lines[1:]])


# ----------------------------------------------------------- code parts --

def words (name):
    return re.sub (r'(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])', ' ', name)


def snake (name):
    return words (name).lower ().replace (' ', '_')


def cpp_string (text):
    return text.replace ('\\', '\\\\').replace ('"', '\\"')


REF_CONVERSIONS = {
    'ElementId': ('const API_Guid {v}Guid = GetGuidFromObjectState (*{v});', '{v}Guid'),
    'Coordinate2D': ('const API_Coord {v}Coord = Get2DCoordinateFromObjectState (*{v});', '{v}Coord'),
    'Coordinate3D': ('const API_Coord3D {v}Coord = Get3DCoordinateFromObjectState (*{v});', '{v}Coord'),
}
GUID_CONVERSION = ('const API_Guid {v}Guid = GetGuidFromObjectState (*{v});', '{v}Guid')


def is_guid_id (ref, common):
    """True for an id definition such as AttributeId: an object holding only a required guid."""
    target = common.get (ref, {})
    return target.get ('type') == 'object' and list (target.get ('properties', {})) == ['guid'] and target.get ('required') == ['guid']


def array_item_id (ref, common):
    """For an ...ArrayItem definition wrapping a guid id (e.g. AttributeIdArrayItem -> 'attributeId'),
    the name of its single field; otherwise None."""
    target = common.get (ref, {})
    props = target.get ('properties', {})
    if len (props) == 1:
        field, sub = next (iter (props.items ()))
        if is_guid_id (sub.get ('$ref', '').split ('/')[-1], common):
            return field
    return None


def or_error_id (ref, common):
    """For an ...OrError definition (oneOf an id array item and ErrorItem), the id field of the
    successful item, e.g. AttributeIdOrError -> 'attributeId'; otherwise None."""
    for option in common.get (ref, {}).get ('oneOf', []):
        field = array_item_id (option.get ('$ref', '').split ('/')[-1], common)
        if field:
            return field
    return None


SCALAR_DECL = {
    'string': ('GS::UniString', '{v}'),
    'number': ('double', '{v} = 0.0'),
    'integer': ('Int32', '{v} = 0'),
    'boolean': ('bool', '{v} = false'),
}


def var_name (field, taken, reserved=()):
    name = field[0].lower () + field[1:]
    if name in {'parameters', 'response', 'item', 'err', 'new', 'delete', 'class'} or name in reserved:
        name += 'Value'
    while name in taken:
        name += '_'
    taken.add (name)
    return name


def field_kind (sub, common):
    """scalar type name, 'array' or 'object' for a property schema."""
    if sub.get ('type') in SCALAR_DECL:
        return sub['type']
    if sub.get ('type') == 'array':
        return 'array'
    if '$ref' in sub:
        target = common.get (sub['$ref'].split ('/')[-1], {})
        if target.get ('type') in SCALAR_DECL:
            return target['type']
        if target.get ('type') == 'array':
            return 'array'
    return 'object'


def array_ctype (sub, common):
    """C++ type that GS::ObjectState::Get reads an array property into: GS::Array<GS::UniString>,
    GS::Array<double>, ... for scalar items, GS::Array<GS::ObjectState> for object items."""
    if '$ref' in sub:
        sub = common.get (sub['$ref'].split ('/')[-1], {})
    items = sub.get ('items', {})
    kind = field_kind (items, common)
    if kind == 'array':
        return f'GS::Array<{array_ctype (items, common)}>'
    if kind in SCALAR_DECL:
        return f'GS::Array<{SCALAR_DECL[kind][0]}>'
    return 'GS::Array<GS::ObjectState>'


def join_names (names):
    names = [repr (n) for n in names]
    return names[0] if len (names) == 1 else ', '.join (names[:-1]) + ' or ' + names[-1]


def gen_field_reads (schema, source, indent, fail, taken, common, reserved=()):
    """Code that reads the fields of an object schema from `source`.
    Required fields are read and checked first; each optional field gets an
    `if (it was given) { TODO }` block, so nothing is left unused and the
    skeleton compiles with warnings as errors.
    fail(message) returns the statement(s) that report a missing field.
    Returns (lines, names of converted values the TODO code still has to use)."""
    pad = ' ' * indent
    props = schema.get ('properties', {})
    required = [f for f in props if f in set (schema.get ('required', []))]
    optional = [f for f in props if f not in required]
    lines, pending = [], []

    def fail_lines (msg, extra):
        return [f'{extra}    {x}' for x in fail (msg).split ('\n')]

    # required values and arrays: read, fail when missing
    pointers = []
    for field in required:
        sub, kind = props[field], field_kind (props[field], common)
        v = var_name (field, taken, reserved)
        if kind == 'object':
            pointers.append ((field, v, sub))
            continue
        if kind == 'array':
            lines.append (f'{pad}{array_ctype (sub, common)} {v};')
        else:
            ctype, init = SCALAR_DECL[kind]
            lines.append (f'{pad}{ctype} {init.format (v=v)};')
        lines.append (f'{pad}if (!{source}.Get ("{field}", {v})) {{')
        lines += fail_lines (f'Invalid or missing {field!r} parameter.', pad)
        lines.append (f'{pad}}}')
    if pointers:
        for field, v, _ in pointers:
            lines.append (f'{pad}const GS::ObjectState* {v} = {source}.Get ("{field}");')
        lines.append (f'{pad}if ({" || ".join (f"{v} == nullptr" for _, v, _ in pointers)}) {{')
        lines += fail_lines (f'Missing {join_names ([f for f, _, _ in pointers])} parameter.', pad)
        lines.append (f'{pad}}}')
        for field, v, sub in pointers:
            ref = sub.get ('$ref', '').split ('/')[-1]
            conversion = REF_CONVERSIONS.get (ref) or (GUID_CONVERSION if is_guid_id (ref, common) else None)
            if conversion:
                code, converted = conversion
                lines.append (pad + code.format (v=v))
                taken.add (converted.format (v=v))
                pending.append (converted.format (v=v))

    # optional values: one block each
    for field in optional:
        sub, kind = props[field], field_kind (props[field], common)
        v = var_name (field, taken, reserved)
        if kind == 'object':
            lines.append (f'{pad}const GS::ObjectState* {v} = {source}.Get ("{field}");')
            lines.append (f'{pad}if ({v} != nullptr) {{')
        else:
            if kind == 'array':
                lines.append (f'{pad}{array_ctype (sub, common)} {v};')
            else:
                ctype, init = SCALAR_DECL[kind]
                lines.append (f'{pad}{ctype} {init.format (v=v)};')
            lines.append (f'{pad}if ({source}.Get ("{field}", {v})) {{')
        lines.append (f'{pad}    // TODO: apply {field}')
        lines.append (f'{pad}}}')
    return lines, pending


def unused_lines (names, indent):
    pad = ' ' * indent
    if not names:
        return []
    return [f'{pad}// Required values; remove each (void) line once the value is used.'] + [f'{pad}(void) {n};' for n in names]


def gen_execute (cmd, input_schema, output_schema, common):
    name = cmd['name']
    cls = name + 'Command'
    guard = cmd.get ('requires')
    undoable = cmd.get ('undoable', not name.startswith ('Get'))
    body = []
    taken = set ()

    out_props = output_schema.get ('properties', {})
    result_list = None
    result_id = None  # id field of the successful item of an ...OrError result list
    if set (out_props) == {'executionResults'}:
        result_list = 'executionResults'
    elif set (out_props) == {'elements'} and out_props['elements'].get ('$ref', '').endswith ('ElementIdsOrErrors'):
        result_list = 'elements'
    elif len (out_props) == 1:
        field, sub = next (iter (out_props.items ()))
        item_ref = sub.get ('items', {}).get ('$ref', '').split ('/')[-1] if sub.get ('type') == 'array' else ''
        if item_ref.endswith ('OrError'):
            result_list = field
            result_id = or_error_id (item_ref, common)

    def object_items (array_schema):
        """The object schema of the array's items, inline or a common definition; else None."""
        items = array_schema.get ('items', {})
        if items.get ('type') == 'object':
            return items
        target = common.get (items.get ('$ref', '').split ('/')[-1], {})
        return target if target.get ('type') == 'object' and 'properties' in target else None

    # batch: one required array of objects in, one result per item out
    batch_field = None
    if input_schema and result_list:
        arrays = [f for f, s in input_schema['properties'].items ()
                  if s.get ('type') == 'array' and object_items (s) is not None
                  and f in input_schema.get ('required', [])]
        if len (arrays) == 1 and len (input_schema['properties']) == 1:
            batch_field = arrays[0]

    top_fail = lambda msg: f'return CreateErrorResponse (APIERR_BADPARS, "{cpp_string (msg)}");'

    if batch_field:
        items_schema = object_items (input_schema['properties'][batch_field])
        # the input array may share its name with the result list (e.g. "elements" in and out)
        batch_var = var_name (batch_field, taken, reserved={result_list})
        body.append (f'    GS::Array<GS::ObjectState> {batch_var};')
        body.append (f'    if (!parameters.Get ("{batch_field}", {batch_var})) {{')
        body.append (f'        {top_fail (f"Invalid or missing {batch_field!r} parameter.")}')
        body.append ('    }')
        body.append ('')
        body.append ('    GS::ObjectState response;')
        body.append (f'    const auto& {result_list} = response.AddList<GS::ObjectState> ("{result_list}");')
        body.append ('')
        loop_indent = 12 if undoable else 8
        if result_list == 'executionResults':
            item_fail = lambda msg: f'executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "{cpp_string (msg)}"));\ncontinue;'
            on_failure = '// executionResults (CreateFailedExecutionResult (err, "...")); continue;'
            created, success = None, 'executionResults (CreateSuccessfulExecutionResult ());'
        elif result_list == 'elements':
            item_fail = lambda msg: f'elements (CreateErrorResponse (APIERR_BADPARS, "{cpp_string (msg)}"));\ncontinue;'
            on_failure = '// elements (CreateErrorResponse (err, "...")); continue;'
            created = 'const API_Guid newElementGuid = APINULLGuid; // TODO: the guid of the created element'
            success = 'elements (CreateElementIdObjectState (newElementGuid));'
        else:
            item_fail = lambda msg: f'{result_list} (CreateErrorResponse (APIERR_BADPARS, "{cpp_string (msg)}"));\ncontinue;'
            on_failure = f'// {result_list} (CreateErrorResponse (err, "...")); continue;'
            if result_id:
                created = 'const API_Guid newGuid = APINULLGuid; // TODO: the guid of the created item'
                success = f'{result_list} (GS::ObjectState ("{result_id}", CreateGuidObjectState (newGuid)));'
            else:
                created, success = None, f'{result_list} (GS::ObjectState ()); // TODO: the result of this item'
        reads, used = gen_field_reads (items_schema, 'item', loop_indent, item_fail, taken, common, reserved={result_list})
        inner = reads + [''] + unused_lines (used, loop_indent) + [
            ' ' * loop_indent + f'// TODO: call the Archicad API for this item. On failure report it with',
            ' ' * loop_indent + on_failure,
        ]
        if created:
            inner.append (' ' * loop_indent + created)
        inner.append (' ' * loop_indent + success)
        loop = [' ' * (loop_indent - 4) + f'for (const GS::ObjectState& item : {batch_var}) {{'] + inner + [' ' * (loop_indent - 4) + '}']
        if undoable:
            body.append (f'    ACAPI_CallUndoableCommand ("{cpp_string (words (name))}", [&] () -> GSErrCode {{')
            body += loop
            body.append ('        return NoError;')
            body.append ('    });')
        else:
            body += loop
        body.append ('')
        body.append ('    return response;')
    else:
        used = []
        if input_schema:
            reads, used = gen_field_reads (input_schema, 'parameters', 4, top_fail, taken, common, reserved=set (out_props))
            body += reads + ['']
        body.append ('    GS::ObjectState response;')
        for field, sub in out_props.items ():
            if sub.get ('type') == 'array':
                item_type = array_ctype (sub, common)[len ('GS::Array<'):-1]
                body.append (f'    const auto& {field}List = response.AddList<{item_type}> ("{field}");')
                body.append (f'    (void) {field}List; // TODO: add one {item_type} per item: {field}List (item);')
            else:
                body.append (f'    // TODO: response.Add ("{field}", ...);')
        body += unused_lines (used, 4)
        if undoable:
            body.append (f'    ACAPI_CallUndoableCommand ("{cpp_string (words (name))}", [&] () -> GSErrCode {{')
            body.append ('        // TODO: call the Archicad API here.')
            body.append ('        return NoError;')
            body.append ('    });')
        else:
            body.append ('    // TODO: call the Archicad API here.')
        body.append ('')
        body.append ('    return response;')

    params_arg = 'parameters' if input_schema else '/*parameters*/'
    lines = [f'GS::ObjectState {cls}::Execute (const GS::ObjectState& {params_arg}, GS::ProcessControl& /*processControl*/) const', '{']
    if guard:
        macro = f'ServerMainVers_{int (guard)}00'
        lines.append (f'#ifdef {macro}')
        lines += body
        lines.append ('#else')
        if input_schema:
            lines.append ('    UNUSED_PARAMETER (parameters);')
        lines.append (f'    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad {int (guard)} or newer.");')
        lines.append ('#endif')
    else:
        lines += body
    lines.append ('}')
    return '\n'.join (lines)


def gen_cpp (cmd, input_schema, output_schema, common):
    cls = cmd['name'] + 'Command'
    common_flag = 'Used' if cmd.get ('commonSchema', True) else 'NotUsed'
    parts = [
        f'{cls}::{cls} () :\n    CommandBase (CommonSchema::{common_flag})\n{{\n}}',
        f'GS::String {cls}::GetName () const\n{{\n    return "{cmd["name"]}";\n}}',
    ]
    if input_schema:
        parts.append (f'GS::Optional<GS::UniString> {cls}::GetInputParametersSchema () const\n{{\n    return R"({schema_literal (input_schema)})";\n}}')
    parts.append (f'GS::Optional<GS::UniString> {cls}::GetRawResponseSchema () const\n{{\n    return R"({schema_literal (output_schema)})";\n}}')
    parts.append (gen_execute (cmd, input_schema, output_schema, common))
    return '\n\n'.join (parts) + '\n'


def gen_hpp (cmd, input_schema):
    cls = cmd['name'] + 'Command'
    lines = [f'class {cls} : public CommandBase', '{', 'public:', f'    {cls} ();',
             '    virtual GS::String GetName () const override;']
    if input_schema:
        lines.append ('    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;')
    lines += ['    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;',
              '    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;',
              '};']
    return '\n'.join (lines) + '\n'


def placeholder (schema, common):
    if '$ref' in schema:
        ref = schema['$ref'].split ('/')[-1]
        if ref in ('Coordinate2D',):
            return {'x': 0.0, 'y': 0.0}
        if ref in ('Coordinate3D',):
            return {'x': 0.0, 'y': 0.0, 'z': 0.0}
        if ref.endswith ('Id') and ref in common and 'guid' in json.dumps (common[ref]):
            return {'guid': '00000000-0000-0000-0000-000000000000'}
        if ref in common:
            return placeholder (common[ref], common)
        return None
    t = schema.get ('type')
    if 'enum' in schema:
        return schema['enum'][0]
    if t == 'string':
        return 'text'
    if t == 'number':
        return 0.0
    if t == 'integer':
        return 0
    if t == 'boolean':
        return False
    if t == 'array':
        return [placeholder (schema.get ('items', {}), common)]
    if t == 'object':
        return {k: placeholder (v, common) for k, v in schema.get ('properties', {}).items () if k in schema.get ('required', [])}
    return None


def gen_example (cmd, input_schema, common):
    params = placeholder (input_schema, common) if input_schema else None
    req = f"# Requires Archicad {cmd['requires']} or newer.\n" if cmd.get ('requires') else ''
    call = (f"result = aclib.RunTapirCommand ('{cmd['name']}', {json.dumps (params, indent=4)})" if params is not None
            else f"result = aclib.RunTapirCommand ('{cmd['name']}')")
    return (f"import aclib\n\n{req}# {cmd.get ('description', '')}\n"
            f"# TODO: replace the placeholder values with real ones.\n\n{call}\n\nprint (result)\n")


def gen_check (cmd, input_schema, output_schema, version, common):
    """A starter check: one call of the command and expectations from its output shape. The params
    are placeholders; the elements a command needs are made in earlier steps of the same check."""
    params = (placeholder (input_schema, common) if input_schema else None) or {}
    props = (output_schema or {}).get ('properties', {})
    out = cmd.get ('output', 'executionResults')
    step = {'id': 'call', 'command': cmd['name'], 'params': params}
    if out == 'elements' or 'elements' in props:
        step['expect'] = [{'path': 'elements[0].elementId.guid', 'exists': True, 'name': 'an element is created'}]
    else:
        step['creates'] = False
        if out == 'executionResults' or 'executionResults' in props:
            step['expect'] = [{'path': 'executionResults[0].success', 'eq': True, 'name': 'succeeds'}]
        else:
            step['expect'] = [{'path': name, 'exists': True} for name in props] or [{'path': 'error', 'exists': False}]
    check = {
        'title': f"{cmd['name']}: TODO what this check proves",
        'minTapir': version,
        '_todo': ('Replace the placeholder params with real ones: make the elements, attributes or stories '
                  'the command needs in earlier steps, save their ids ("save": {"w1": "elements[0].elementId.guid"}) '
                  'and use "${w1}". Then make the expectations prove the behaviour (eq, approx, len, ...), not '
                  'only that it answers. Format: tools/community-test-runner/README.md, "Writing checks". '
                  'Remove this note when done.'),
        'steps': [step],
    }
    return json.dumps (check, indent=2) + '\n'


# ------------------------------------------------------------ insertion --

NEWLINES = {}  # path -> line ending found on read, kept on write


def read (path):
    with open (path, encoding='utf-8', newline='') as f:
        text = f.read ()
    NEWLINES[os.path.abspath (path)] = '\r\n' if '\r\n' in text else '\n'
    return text.replace ('\r\n', '\n')


def write (path, text, dry, label):
    if dry:
        return
    newline = NEWLINES.get (os.path.abspath (path), '\r\n' if os.name == 'nt' else '\n')
    with open (path, 'w', encoding='utf-8', newline=newline) as f:
        f.write (text)


def registration_block (cmd, version):
    return (f'        err |= RegisterCommand<{cmd["name"]}Command> (\n'
            f'            {cmd["group"]}, "{version}",\n'
            f'            "{cpp_string (cmd["description"])}"\n'
            f'        );\n')


def insert_registration (main_text, cmd, version):
    group = cmd['group']
    m = re.search (r'^[ \t]*AddCommandGroup \(' + re.escape (group) + r'\);[ \t]*$', main_text, re.M)
    if not m or not re.search (r'CommandGroup ' + re.escape (group) + r' \(', main_text):
        groups = re.findall (r'CommandGroup (\w+) \(', main_text)
        raise SpecError (f'{cmd["name"]}: no command group "{group}" in AddOnMain.cpp. Known groups: {", ".join (groups)}')
    return main_text[:m.start ()] + registration_block (cmd, version) + main_text[m.start ():]


def validate (cmd, i):
    for key in ('name', 'file', 'group', 'description'):
        if not cmd.get (key):
            raise SpecError (f'command #{i + 1}: missing "{key}"')
    if not re.fullmatch (r'[A-Z][A-Za-z0-9]*', cmd['name']):
        raise SpecError (f'{cmd["name"]}: the name must be CamelCase')
    if not re.fullmatch (r'[A-Za-z0-9]+', cmd['file']):
        raise SpecError (f'{cmd["name"]}: "file" is a Sources file name without extension, e.g. MEPCommands')


def load_spec (path):
    text = read (path)
    if path.endswith (('.yaml', '.yml')):
        try:
            import yaml
        except ImportError:
            raise SpecError ('PyYAML is not installed; install it or write the spec as JSON')
        data = yaml.safe_load (text)
    else:
        data = json.loads (text)
    if isinstance (data, dict):
        data = data.get ('commands', [data])
    return data


def main ():
    ap = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    ap.add_argument ('spec')
    ap.add_argument ('--dry-run', action='store_true', help='print the generated code, write nothing')
    ap.add_argument ('--schemas', action='store_true', help='print only the expanded schemas as JSON')
    args = ap.parse_args ()

    try:
        commands = load_spec (args.spec)
        names = [cmd.get ('name') for cmd in commands]
        twice = sorted ({n for n in names if n and names.count (n) > 1})
        if twice:
            raise SpecError (f'the spec has {join_names (twice)} more than once')
        version = json.loads (read (PACKAGE_INFO))['version']
        common = json.loads (read (os.path.join (SOURCES, 'RFIX', 'Images', 'CommonSchemaDefinitions.json')))
        main_text = read (ADDON_MAIN)
        main_original = main_text
        all_sources = '\n'.join (read (os.path.join (SOURCES, f)) for f in os.listdir (SOURCES) if f.endswith ('.hpp'))

        pending = {}  # file -> [hpp, cpp]
        written, skipped = [], []
        schemas_out = {}
        for i, cmd in enumerate (commands):
            validate (cmd, i)
            name = cmd['name']
            input_schema = expand (cmd['input'], name + '.input') if cmd.get ('input') else None
            if input_schema and input_schema.get ('type') != 'object':
                raise SpecError (f'{name}: "input" must be an object')
            out = cmd.get ('output', 'executionResults')
            output_schema = expand (OUTPUT_SHORTCUTS.get (out, out) if isinstance (out, str) else out, name + '.output')
            for ref in re.findall (r'"#/(\w+)"', json.dumps ([input_schema, output_schema])):
                if ref not in common:
                    raise SpecError (f'{name}: "#{ref}" is not in CommonSchemaDefinitions.json')
            schemas_out[name] = {'input': input_schema, 'output': output_schema}
            if args.schemas:
                continue
            if re.search (r'\bclass ' + name + r'Command\b', all_sources):
                skipped.append (name)
                continue
            hpp_path = os.path.join (SOURCES, cmd['file'] + '.hpp')
            cpp_path = os.path.join (SOURCES, cmd['file'] + '.cpp')
            if cmd['file'] not in pending:
                if os.path.exists (hpp_path) != os.path.exists (cpp_path):
                    raise SpecError (f'{name}: only one of {cmd["file"]}.hpp/.cpp exists')
                if os.path.exists (hpp_path):
                    pending[cmd['file']] = [read (hpp_path), read (cpp_path), False]
                else:
                    pending[cmd['file']] = ['#pragma once\n\n#include "CommandBase.hpp"\n',
                                            f'#include "{cmd["file"]}.hpp"\n', True]
            entry = pending[cmd['file']]
            hpp_part, cpp_part = gen_hpp (cmd, input_schema), gen_cpp (cmd, input_schema, output_schema, common)
            entry[0] = entry[0].rstrip ('\n') + '\n\n' + hpp_part
            entry[1] = entry[1].rstrip ('\n') + '\n\n' + cpp_part
            if args.dry_run:
                print (f'===== {cmd["file"]}.hpp\n{hpp_part}\n===== {cmd["file"]}.cpp\n{cpp_part}')
                print (f'===== AddOnMain.cpp\n{registration_block (cmd, cmd.get ("version", version))}')
            main_text = insert_registration (main_text, cmd, cmd.get ('version', version))
            written.append (name)
            if cmd.get ('example', True):
                ex_path = os.path.join (EXAMPLES, snake (name) + '.py')
                if os.path.exists (ex_path):
                    print (f'note: {os.path.relpath (ex_path, ROOT)} exists, example not written')
                else:
                    if args.dry_run:
                        print (f'===== Examples/{snake (name)}.py\n{gen_example (cmd, input_schema, common)}')
                    write (ex_path, gen_example (cmd, input_schema, common), args.dry_run, 'example')
            if cmd.get ('check', True):
                check_path = os.path.join (CHECKS, snake (name) + '.json')
                check_text = gen_check (cmd, input_schema, output_schema, cmd.get ('version', version), common)
                if os.path.exists (check_path):
                    print (f'note: {os.path.relpath (check_path, ROOT)} exists, check not written')
                else:
                    if args.dry_run:
                        print (f'===== Test/checks/{snake (name)}.json\n{check_text}')
                    elif not os.path.isdir (CHECKS):
                        os.makedirs (CHECKS)
                    write (check_path, check_text, args.dry_run, 'check')

        if args.schemas:
            print (json.dumps (schemas_out, indent=4))
            return 0

        for file, (hpp, cpp, new) in pending.items ():
            if new:
                include = f'#include "{file}.hpp"\n'
                last = list (re.finditer (r'^#include "[^"]+Commands\.hpp"\n', main_text, re.M))[-1]
                main_text = main_text[:last.end ()] + include + main_text[last.end ():]
            write (os.path.join (SOURCES, file + '.hpp'), hpp, args.dry_run, 'header')
            write (os.path.join (SOURCES, file + '.cpp'), cpp, args.dry_run, 'source')
        if main_text != main_original:
            write (ADDON_MAIN, main_text, args.dry_run, 'registration')

        print (f'generated: {", ".join (written) or "nothing"}')
        if skipped:
            print (f'skipped (already exist): {", ".join (skipped)}')
        if written and not args.dry_run:
            print ('next: fill the TODOs in Execute and in the check file (Test/checks/), then run '
                   'python tools/generate_addon_docs.py to check the schemas parse '
                   '(revert docs/ afterwards: the weekly release regenerates them)')
        return 0
    except SpecError as e:
        print (f'error: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit (main ())
