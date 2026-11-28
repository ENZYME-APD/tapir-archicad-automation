"""Generate the field coverage of the Tapir Add-On: which element settings have a command key of their own.

The API coverage checklist (tools/generate_api_coverage.py) counts an ACAPI_*
function as used once Tapir calls it, so a command calling ACAPI_Element_Change
counts even when it leaves most of an element's settings out (#2883). This
lists every field of every element type and says how a script reaches it.

The fields are the rows of archicad-addon/Sources/ElementFieldTable.cpp, which
tools/generate_element_field_table.py generates from the DevKit headers: every
leaf field of every struct of the API_Element union (API_WallType.thickness,
API_RoofType.shellBase.topMat, ...), without padding, reserved members and
pointers. GetElementFields and SetElementFields read and write any of them by
its path, on the Archicad versions that table has (Archicad 29 and 30), so on
those versions every field is reachable. What this report adds is whether a
command also has a key of its own for it, the way CreateWalls has thickness:
that is what a script uses on every Archicad version, with validation and a
readable name.

Status of a field:

- key, written: code outside the field table writes it by name (assigns it or
  sets its change mask), so a command most likely takes it;
- key, read: code reads it by name and none writes it, so a command most likely
  returns it;
- generic only: no code names it; GetElementFields reads it and, unless the
  table marks it read only, SetElementFields writes it;
- read only: no code names it, and the table marks it read only (owners and
  counts Archicad computes, and the fields that choose an element's shape);
- not needed: no code names it, and tools/field_coverage_rules.json rules it out,
  with a reason (a field that describes one placed sub-element rather than a
  setting, for example). A field that code names keeps its key status whatever a
  rule says, and a rule that matches no generic-only field is reported, so a rule
  that a new key has made stale is found.

The share of fields with a key leaves out the read only and not needed ones,
which can have no key or need none.

It is a static estimate, not a test. A field counts as named when the code
reaches it or one of its members (topMat.hasValue)

- through the union (element.wall.thickness, elem->roof.shellBase.topMat),
- through a variable or parameter of an element struct, or of the type of a
  struct it holds when only a few element structs hold that type, in the same
  function (const API_ShellBaseType& base = roof.shellBase; base.topMat),
- or in a change mask (ACAPI_ELEMENT_MASK_SET (mask, API_WallType, thickness)).

Code that assigns or hands over a whole struct the field is in counts too
(element.beam.begC = c; a story visibility passed to a helper), unless the
struct's type is one followed through typed variables: then the helper's own
accesses are what counts (handing a stair's structure to a helper names none of
its fields). Elements of an array match an indexed access (structure[i][j]),
and an alias type (API_DoorType is API_WindowType) shares the coverage of the
type it names.

So "written" means some code writes it, not that a command takes it as input,
and "read" means some code reads it, not that GetDetailsOfElements returns it.

Standard library only. Usage:

    python tools/generate_field_coverage.py \
        --devkit 29=Build/DevKits/AC29/Support --devkit 30=Build/DevKits/AC30/Support \
        --sources archicad-addon/Sources \
        --out-dir out

--rules gives another rules file than tools/field_coverage_rules.json.

The newest given DevKit that ElementFieldTable.cpp has a table for is used.

Writes out/field-coverage.json, out/field-coverage.md and
out/field-coverage-summary.md.
"""

import argparse
import collections
import datetime
import json
import pathlib
import re

FIELD_TABLE = 'ElementFieldTable.cpp'
# The generic field commands use the table, not the names.
GENERIC_SOURCES = {'ElementFieldTable.cpp', 'ElementFieldTable.hpp', 'ElementFieldCommands.cpp', 'ElementFieldCommands.hpp'}
STATUS_ORDER = ['key, written', 'key, read', 'generic only', 'not needed', 'read only']
STATUS_MARK = {'key, written': '✏️', 'key, read': '👁️', 'generic only': '🔧', 'not needed': '➖', 'read only': '🔒'}
# Left out of the share of fields with a key: they can have no key, or need none.
NOT_APPLICABLE = ('not needed', 'read only')
RULES = pathlib.Path (__file__).resolve ().parent / 'field_coverage_rules.json'

IDENT = r'[A-Za-z_][A-Za-z0-9_]*'
CHAIN = r'((?:\s*(?:\.|->)\s*' + IDENT + r'(?:\s*\[[^\]\n]*\])*)*)'
ROW_RE = re.compile (r'\{\s*"([^"]+)",\s*TF_OFF\s*\(\s*(API_[A-Za-z0-9_]+)\s*,[^)]*\)\s*,\s*TF_SIZE\s*\([^)]*\)\s*,\s*FieldKind::(\w+)\s*,\s*(true|false)\s*,\s*(nullptr|"[^"]*")')
TABLE_RE = re.compile (r'static const FieldTable table = \{\s*"(API_[A-Za-z0-9_]+)",\s*"(' + IDENT + r')"')
STRUCT_START_RE = re.compile (r'\b(?:struct|union)\s+(API_[A-Za-z0-9_]+)\s*(?::[^{;]*)?\{')
MASK_RE = re.compile (r'\bACAPI_ELEMENT_MASK_(SET|CHECK)\s*\(\s*[^,()]+,\s*(API_[A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_.\[\]]+)\s*\)')
MAX_TYPED_OWNERS = 6
COMMON_TYPES = {'API_Coord', 'API_Coord3D', 'API_Box', 'API_Box3D', 'API_Guid', 'API_Vector', 'API_Vector3D', 'API_Tranmat',
                'API_RGBColor', 'API_Elem_Head', 'API_OverriddenAttribute', 'API_OverriddenPen', 'API_ExtendedPenType',
                'API_Polygon', 'API_ElemType', 'API_Sector', 'API_UVCoord', 'API_StoryVisibility', 'API_LinkToSettings'}
WRITE_AFTER_RE = re.compile (r'\s*(?:\[[^\]\n]*\]\s*)*(?:[-+*/|&^]?=(?!=)|\+\+|--)')


def read_text (path):
    return path.read_text (encoding='utf-8', errors='replace')


def blank_comments_and_strings (text):
    """The text with comments and string or character literals replaced by spaces, newlines kept."""
    out = []
    i, n = 0, len (text)
    while i < n:
        c = text[i]
        if text.startswith ('//', i):
            j = text.find ('\n', i)
            j = n if j < 0 else j
            out.append (' ' * (j - i))
            i = j
        elif text.startswith ('/*', i):
            j = text.find ('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append (re.sub (r'[^\n]', ' ', text[i:j]))
            i = j
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min (j + 1, n)
            out.append (' ' * (j - i))
            i = j
        else:
            out.append (c)
            i += 1
    return ''.join (out)


def matching_brace (text, open_pos):
    depth = 0
    for i in range (open_pos, len (text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return i
    return len (text)


def read_field_table (path, version):
    """{struct: [{'path', 'kind', 'readOnly', 'variant'}]} and {struct: [union members]} of one version's table."""
    text = read_text (path)
    start = text.find (f'defined (ServerMainVers_{version}00) && !defined (ServerMainVers_{version + 1}00)')
    if start < 0:
        raise SystemExit (f'{path} has no table for Archicad {version}')
    end = min ((p for p in (text.find ('\n#elif', start), text.find ('\n#else', start)) if p >= 0), default=len (text))
    section = text[start:end]
    fields = collections.defaultdict (list)
    for m in ROW_RE.finditer (section):
        if not any (f['path'] == m.group (1) for f in fields[m.group (2)]):
            fields[m.group (2)].append ({'path': m.group (1), 'kind': m.group (3), 'readOnly': m.group (4) == 'true',
                                         'variant': None if m.group (5) == 'nullptr' else m.group (5).strip ('"')})
    members = collections.defaultdict (list)
    for m in TABLE_RE.finditer (section):
        if m.group (2) not in members[m.group (1)]:
            members[m.group (1)].append (m.group (2))
    return fields, members


def type_aliases (support):
    """{name: name}: the element struct names that are another name of one (using API_DoorType = API_WindowType;)."""
    aliases = {}
    for path in sorted ((support / 'Inc').rglob ('*.h*')):
        text = blank_comments_and_strings (read_text (path))
        for m in re.finditer (r'\busing\s+(API_[A-Za-z0-9_]+)\s*=\s*(API_[A-Za-z0-9_]+)\s*;', text):
            aliases[m.group (1)] = m.group (2)
        for m in re.finditer (r'\btypedef\s+(API_[A-Za-z0-9_]+)\s+(API_[A-Za-z0-9_]+)\s*;', text):
            aliases[m.group (2)] = m.group (1)
    return aliases


def struct_bodies (support):
    """{struct name: blanked body} of every API_ struct and union of the DevKit's Inc headers."""
    bodies = {}
    for path in sorted ((support / 'Inc').rglob ('*.h*')):
        text = blank_comments_and_strings (read_text (path))
        for m in STRUCT_START_RE.finditer (text):
            bodies.setdefault (m.group (1), text[m.end ():matching_brace (text, m.end () - 1)])
    return bodies


def struct_typed_members (body):
    """[(type, path)] of the members of a struct body whose type is an API_ struct, arrays and nested unions included."""
    found = []
    depth, start = 0, 0
    for i, c in enumerate (body):
        if c == '{':
            if depth == 0:
                block_start = i
            depth += 1
        elif c == '}':
            depth -= 1
        elif c == ';' and depth == 0:
            statement = body[start:i]
            if '{' in statement:
                # union { ... } u; or struct { ... };
                close = statement.rfind ('}')
                name = statement[close + 1:].strip ()
                prefix = name + '.' if re.fullmatch (IDENT, name) else ''
                found += [(t, prefix + n) for t, n in struct_typed_members (statement[statement.find ('{') + 1:close])]
            else:
                m = re.match (r'\s*(?:const\s+)?(API_[A-Za-z0-9_]+)\s+(' + IDENT + r')\s*(?:\[[^\]]*\]\s*)*$', statement)
                if m:
                    found.append ((m.group (1), m.group (2)))
            start = i + 1
    return found


def nested_member_types (support, structs, max_depth=5):
    """{struct type: [(element struct, path)]}: where the element structs hold a struct of that type, at any depth
    (API_ShellBaseType -> (API_RoofType, 'shellBase'), API_DimBase -> (API_AngleDimensionType, 'base.base'))."""
    bodies = struct_bodies (support)
    nested = collections.defaultdict (list)

    def walk (owner, type_name, prefix, depth):
        if depth > max_depth or type_name not in bodies:
            return
        for member_type, name in struct_typed_members (bodies[type_name]):
            if member_type.endswith (('ID', 'Index')) or member_type == type_name:
                continue
            nested[member_type].append ((owner, prefix + name))
            walk (owner, member_type, prefix + name + '.', depth + 1)

    for struct in structs:
        walk (struct, struct, '', 1)
    return nested


def chain_path (chain):
    """'.shellBase . topMat [0]' -> 'shellBase.topMat'"""
    return '.'.join (re.findall (IDENT, re.sub (r'\[[^\]]*\]', '', chain)))


def function_bodies (blank):
    """(start, end) of the bodies of the outermost functions of a source file."""
    bodies, stack = [], []
    for i, c in enumerate (blank):
        if c == '{':
            in_function = any (stack)
            before = blank[max (0, i - 300):i].rstrip ()
            is_function = (not in_function
                           and re.search (r'\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?(?:->\s*[\w:<>&* ]+)?(?::\s*[^;{}]*)?$', before) is not None
                           and not re.search (r'\b(?:if|for|while|switch|catch)\s*\([^;{}]*\)\s*$', before))
            stack.append (is_function)
            if is_function:
                bodies.append ([i + 1, None])
        elif c == '}' and stack:
            if stack.pop () and not any (stack):
                bodies[-1][1] = i
    return [(s, e) for s, e in bodies if e is not None]


def scan_sources (sources, members, nested):
    """{struct: {path: {'read', 'write'}}}: the paths the sources name, outside the generic field commands."""
    found = collections.defaultdict (lambda: collections.defaultdict (set))
    member_structs = collections.defaultdict (set)
    for struct, names in members.items ():
        for name in names:
            member_structs[name].add (struct)
    union_names = '|'.join (sorted (member_structs, key=len, reverse=True))
    union_re = re.compile (r'(?:\.|->)\s*(' + union_names + r')\b' + CHAIN)
    # A variable or parameter of an element struct, or of the type of a struct-typed member.
    typed = {struct: [(struct, '')] for struct in members}
    for type_name, owners in nested.items ():
        owners = [(struct, member) for struct, member in owners if struct in members]
        # A value type that many elements hold (API_Coord, API_Box) says nothing about which element a
        # helper handles: only a struct type held in a few places identifies the element.
        if not owners or not is_followed (type_name, nested) or type_name in typed:
            continue
        typed[type_name] = [(struct, member + '.') for struct, member in owners]
    decl_re = re.compile (r'\b(' + '|'.join (sorted (typed, key=len, reverse=True)) + r')\b\s*(?:const\s*)?[&*]*\s*(' + IDENT + r')\s*(?=[=;,)({\[])')
    auto_re = re.compile (r'\bauto\s*(?:const\s*)?[&*]*\s*(' + IDENT + r')\s*=\s*[^;{}]*?(?:\.|->)\s*(' + union_names + r')\b' + CHAIN + r'\s*;')

    def record (owners, path, kind):
        if path:
            for struct, prefix in owners:
                found[struct][prefix + path].add (kind)

    def kind_at (text, pos):
        return 'write' if WRITE_AFTER_RE.match (text, pos) else 'read'

    for path in sorted (sources.rglob ('*.[ch]pp')):
        if path.name in GENERIC_SOURCES:
            continue
        blank = blank_comments_and_strings (read_text (path))
        for m in union_re.finditer (blank):
            record ([(s, '') for s in member_structs[m.group (1)]], chain_path (m.group (2)), kind_at (blank, m.end ()))
        for m in MASK_RE.finditer (blank):
            if m.group (2) in members:
                record ([(m.group (2), '')], chain_path ('.' + m.group (3)), 'write' if m.group (1) == 'SET' else 'read')
        for start, end in function_bodies (blank):
            head = blank.rfind ('\n\n', 0, start)
            scope = blank[max (0, head):end]
            aliases = {}
            for m in decl_re.finditer (scope):
                aliases[m.group (2)] = typed[m.group (1)]
            for m in auto_re.finditer (scope):
                prefix = chain_path (m.group (3))
                aliases[m.group (1)] = [(s, prefix + '.' if prefix else '') for s in member_structs[m.group (2)]]
            aliases = {a: o for a, o in aliases.items () if o}
            if not aliases:
                continue
            alias_re = re.compile (r'(?<![\w.>])(' + '|'.join (map (re.escape, aliases)) + r')' + CHAIN)
            for m in alias_re.finditer (scope):
                record (aliases[m.group (1)], chain_path (m.group (2)), kind_at (scope, m.end ()))
    return found


def is_followed (type_name, nested):
    """Whether the code's accesses to a struct of this type are found through its typed variables (see scan_sources)."""
    owners = nested.get (type_name, [])
    return bool (owners) and len (owners) <= MAX_TYPED_OWNERS and type_name not in COMMON_TYPES


def named_kinds (path, named, member_type):
    """What the code does with a field: with the field itself, with one of its members (topMat.hasValue), or
    with a struct the field is in (element.beam.begC = ..., a story visibility handed to a helper).

    A struct the field is in counts only when its type is not followed through typed variables: code
    that hands a stair's structure to a helper names none of its fields, and the helper's own accesses
    are found through its parameter instead. member_type (path -> type) gives the types."""
    # The table writes the elements of an array (structure[0][2].data); the code indexes it with a variable.
    path = re.sub (r'\[[^\]]*\]', '', path)
    kinds = set ()
    for other, k in named.items ():
        if other == path or other.startswith (path + '.'):
            kinds |= k
        elif path.startswith (other + '.') and not member_type (other):
            kinds |= k
    return kinds


def read_rules (path):
    """The not-needed rules of a rules file: [{'struct', 'paths', 'reason', 'matched': 0}]. A path is a field's
    path as the table writes it, or a prefix ending in '.*' for every field under it; array indices are
    ignored, so 'part.pen' matches part[0].pen and part[1].pen."""
    if path is None or not path.is_file ():
        return []
    rules = json.loads (read_text (path)).get ('notNeeded', [])
    for rule in rules:
        if not rule.get ('struct') or not rule.get ('paths') or not rule.get ('reason'):
            raise SystemExit (f'{path}: a notNeeded rule needs struct, paths and reason: {rule}')
        rule['matched'] = 0
    return rules


def rule_matches (pattern, path):
    plain = re.sub (r'\[[^\]]*\]', '', path)
    pattern = re.sub (r'\[[^\]]*\]', '', pattern)
    if pattern.endswith ('.*'):
        return plain.startswith (pattern[:-1])
    return plain == pattern


def not_needed_reason (rules, struct, path):
    for rule in rules:
        if rule['struct'] == struct and any (rule_matches (p, path) for p in rule['paths']):
            rule['matched'] += 1
            return rule['reason']
    return None


def build (support, sources, version, rules=()):
    fields, members = read_field_table (sources / FIELD_TABLE, version)
    nested = nested_member_types (support, set (members) | {t for t in fields})
    found = scan_sources (sources, members, nested)
    # API_DoorType is API_WindowType: code that handles one handles the other.
    same = collections.defaultdict (set)
    for alias, target in type_aliases (support).items ():
        if alias in fields or target in fields:
            same[alias].add (target)
            same[target].add (alias)
    types = []
    for struct in sorted (fields):
        named = collections.defaultdict (set)
        for name in {struct} | same[struct]:
            for path, kinds in found.get (name, {}).items ():
                named[path] |= kinds
        followed = {path for type_name, owners in nested.items () if is_followed (type_name, nested)
                    for owner, path in owners if owner in {struct} | same[struct]}
        rows = []
        for field in fields[struct]:
            kinds = named_kinds (field['path'], named, lambda p: p in followed)
            if 'write' in kinds:
                status = 'key, written'
            elif kinds:
                status = 'key, read'
            elif field['readOnly']:
                status = 'read only'
            else:
                status = 'generic only'
            row = dict (field, status=status)
            if status == 'generic only':
                reason = not_needed_reason (rules, struct, field['path'])
                if reason:
                    row.update (status='not needed', reason=reason)
            rows.append (row)
        types.append ({'struct': struct, 'members': members.get (struct, []), 'fields': rows})
    return types


def percent (part, whole):
    return round (100 * part / whole) if whole else 0


def write_outputs (types, version, out_dir, generated_at=None):
    out_dir.mkdir (parents=True, exist_ok=True)
    now = generated_at or datetime.datetime.now (datetime.timezone.utc).strftime ('%Y-%m-%d %H:%M UTC')
    total = collections.Counter ()
    for t in types:
        t['counts'] = collections.Counter (f['status'] for f in t['fields'])
        total.update (t['counts'])
    (out_dir / 'field-coverage.json').write_text (json.dumps (
        {'generatedAt': now, 'devkit': version, 'counts': total, 'types': types}, indent=1, ensure_ascii=False), encoding='utf-8')

    def keyed (c):
        return c['key, written'] + c['key, read']

    def all_fields (c):
        return sum (c.values ())

    def applicable (c):
        return all_fields (c) - sum (c[s] for s in NOT_APPLICABLE)

    order = sorted (types, key=lambda t: (-all_fields (t['counts']), t['struct']))
    summary = [
        f'**{keyed (total)} of {applicable (total)}** fields of the Archicad element types that a command can write or read '
        f'have a command key of their own ({percent (keyed (total), applicable (total))} %; '
        f'{all_fields (total)} fields in all): '
        + ', '.join (f'{STATUS_MARK[s]} {total[s]} {s}' for s in STATUS_ORDER if total[s]) + '. '
        f'On Archicad 29 and 30, GetElementFields and SetElementFields reach every field by its path.',
        '',
        '| Element type | With a key | Fields | ' + ' | '.join (STATUS_MARK[s] for s in STATUS_ORDER) + ' |',
        '|---|---|---|' + '---|' * len (STATUS_ORDER),
    ]
    for t in order:
        c = t['counts']
        summary.append (f'| [`{t["struct"]}`](#{t["struct"].lower ()}) | {percent (keyed (c), applicable (c))} % | {all_fields (c)} | '
                        + ' | '.join (str (c[s] or '') for s in STATUS_ORDER) + ' |')
    summary += ['', f'Fields of the Archicad {version} DevKit; an estimate read from the code, updated {now}.']
    (out_dir / 'field-coverage-summary.md').write_text ('\n'.join (summary) + '\n', encoding='utf-8')

    lines = [
        '# Archicad element field coverage',
        '',
        'Every field of every Archicad element type, and whether a Tapir command has a key of its own for it. '
        'Generated by `tools/generate_field_coverage.py`; do not edit by hand (#2883).',
        '',
    ] + summary + [
        '',
        '| | Status |',
        '|---|---|',
        f'| {STATUS_MARK["key, written"]} | code writes it by name: a command most likely takes it |',
        f'| {STATUS_MARK["key, read"]} | code reads it by name, none writes it: a command most likely returns it |',
        f'| {STATUS_MARK["generic only"]} | no key of its own: only GetElementFields / SetElementFields (Archicad 29 and 30) |',
        f'| {STATUS_MARK["not needed"]} | no key of its own, and ruled out in `tools/field_coverage_rules.json`, with the reason given |',
        f'| {STATUS_MARK["read only"]} | no key of its own, and read only: Archicad computes it, or it chooses the element\'s shape |',
        '',
        'How it is read: a field counts as named when the code reaches it, a struct it is in, or one of its members, '
        'through the union (`element.wall.thickness`), through a variable or parameter of the struct\'s type in the same '
        'function, or in a change mask. "Written" does not prove a command takes it as input, and "read" does not prove '
        '`GetDetailsOfElements` returns it. The 🔧 rows are the backlog of settings a script can only reach through the '
        'generic field commands, or not at all before Archicad 29.',
        '',
    ]
    for t in order:
        c = t['counts']
        names = ', '.join (f'`{m}`' for m in t['members'])
        lines += [f'## {t["struct"]}', '', f'{names}: {keyed (c)} of {applicable (c)} fields have a key ({all_fields (c)} in all).', '']
        for f in t['fields']:
            text = f'- {STATUS_MARK[f["status"]]} `{f["path"]}`'
            if f['variant']:
                text += f' <sub>only when the element uses `{f["variant"]}`</sub>'
            if f.get ('reason'):
                text += f' – not needed: {f["reason"]}'
            lines.append (text)
        lines.append ('')
    (out_dir / 'field-coverage.md').write_text ('\n'.join (lines), encoding='utf-8')
    return total


def table_versions (path):
    """The Archicad versions ElementFieldTable.cpp has a table for."""
    return sorted ({int (v) for v in re.findall (r'defined \(ServerMainVers_(\d+)00\) && !defined', read_text (path))})


def main ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ('--devkit', action='append', required=True, metavar='VERSION=SUPPORT_DIR',
                         help='a DevKit; the newest one that ElementFieldTable.cpp has a table for is used')
    parser.add_argument ('--sources', required=True, type=pathlib.Path)
    parser.add_argument ('--out-dir', required=True, type=pathlib.Path)
    parser.add_argument ('--rules', type=pathlib.Path, default=RULES,
                         help='the not-needed rules (default: tools/field_coverage_rules.json)')
    args = parser.parse_args ()
    devkits = {int (v): pathlib.Path (p) for v, p in (d.split ('=', 1) for d in args.devkit)}
    usable = sorted (set (devkits) & set (table_versions (args.sources / FIELD_TABLE)))
    if not usable:
        parser.error (f'{FIELD_TABLE} has a table for none of the given DevKits')
    version = usable[-1]
    rules = read_rules (args.rules)
    types = build (devkits[version], args.sources, version, rules)
    total = write_outputs (types, version, args.out_dir)
    print (f'Archicad {version}: {len (types)} element types, {sum (total.values ())} fields: '
           + ', '.join (f'{total[s]} {s}' for s in STATUS_ORDER if total[s]))
    # A rule that matches nothing names a field that has a key now, or that the table no longer has.
    for rule in rules:
        if not rule['matched']:
            print (f'warning: the notNeeded rule for {rule["struct"]} {rule["paths"]} matches no generic-only field')


if __name__ == '__main__':
    main ()
