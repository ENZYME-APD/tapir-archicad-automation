"""Generate archicad-addon/Sources/ElementFieldTable.cpp: every field of every element struct of the
API_Element union, for the GetElementFields, SetElementFields and GetElementFieldDefinitions commands.

The headers of one DevKit are parsed; each leaf field (a number, a bool, an enum, an attribute index,
a guid or a character array) becomes one row with its path, written as C++ so that the compiler
computes its offset and size (offsetof, sizeof): a path that does not exist makes the build fail
instead of reading the wrong bytes. Nested structs, named and anonymous unions and small fixed
arrays are expanded; pointers, memo handles, GS containers, reserved and filler members are left out.

Standard library only. Usage, with the DevKits of devkits.json (the releases the build checks use):

    python tools/generate_element_field_table.py 29=Build/DevKits/AC29/Support 30=Build/DevKits/AC30/Support archicad-addon/Sources/ElementFieldTable.cpp

Each version's table is generated from its own headers and compiled only for that version
(#if ServerMainVers_xx00); a version with no table makes the commands answer that it is not available.
"""

import pathlib
import re
import sys

MAX_DEPTH = 7
MAX_ARRAY = 16

SIGNED = {'char', 'signed char', 'short', 'short int', 'int', 'long', 'long int', 'long long', 'Int8', 'Int16', 'Int32', 'Int64',
          'GS::Int8', 'GS::Int16', 'GS::Int32', 'GS::Int64', 'GSSize', 'GS::Int', 'GSErrCode', 'GS::IntPtr', 'GS::Short'}
UNSIGNED = {'unsigned char', 'unsigned short', 'unsigned short int', 'unsigned int', 'unsigned', 'unsigned long', 'unsigned long long',
            'UInt8', 'UInt16', 'UInt32', 'UInt64', 'GS::UInt8', 'GS::UInt16', 'GS::UInt32', 'GS::UInt64', 'GSFlags', 'GS::UShort',
            'UShort', 'size_t', 'GS::USize', 'GS::UIntPtr', 'GSType', 'GS::uchar_t'}
REAL = {'double', 'float', 'GS::Float', 'GS::Double'}
BOOL = {'bool', 'GSBool', 'Boolean'}
CHARS = {'char', 'signed char', 'unsigned char', 'Int8', 'UInt8', 'GS::Int8', 'GS::UInt8'}
UCHARS = {'GS::uchar_t', 'GS::UniChar::Layout', 'unsigned short', 'UInt16', 'GS::UInt16'}

ELEM_ID_EXCEPTIONS = {
    'cwFrame': 'API_CurtainWallFrameID', 'cwPanel': 'API_CurtainWallPanelID', 'cwSegment': 'API_CurtainWallSegmentID',
    'cwJunction': 'API_CurtainWallJunctionID', 'cwAccessory': 'API_CurtainWallAccessoryID', 'camset': 'API_CamSetID',
    'sectElem': 'API_SectElemID', 'polyLine': 'API_PolyLineID', 'stairRiser': 'API_RiserID', 'stairTread': 'API_TreadID',
}


def find_matching (text, i):
    depth = 0
    for j in range (i, len (text)):
        if text[j] == '{':
            depth += 1
        elif text[j] == '}':
            depth -= 1
            if depth == 0:
                return j
    return -1


def strip_comments (t):
    t = re.sub (r'/\*.*?\*/', ' ', t, flags=re.S)
    return re.sub (r'//[^\n]*', ' ', t)


structs = {}       # name -> [(type, name, arraySizes, isPointer)]
union_types = set ()    # the synthetic types of named unions ("union { ... } u;")
anon_counter = [0]


def parse_members (body, owner_name):
    """Members of a struct body (comments removed). Named nested unions/structs get a synthetic type."""
    members = []
    i = 0
    stmt = ''
    while i < len (body):
        c = body[i]
        if c == '{':
            j = find_matching (body, i)
            if j < 0:
                break
            head = stmt.strip ()
            after = body[j + 1:]
            k = after.find (';')
            tail = after[:k].strip () if k >= 0 else ''
            if '(' in head or head.startswith (('enum', 'class')):
                # A method body, a nested enum or class: no members.
                i = j + 1 if '(' in head or k < 0 else j + 1 + k + 1
            elif head in ('', 'union', 'struct') or re.match (r'^(union|struct)\s+\w*$', head):
                inner = parse_members (body[i + 1:j], owner_name)
                if tail and re.match (r'^\w+(\[[^\]]*\])*$', tail):
                    anon_counter[0] += 1
                    synth = f'<anon{anon_counter[0]}>'
                    structs[synth] = inner
                    if head.startswith ('union'):
                        union_types.add (synth)
                    nm = re.match (r'^(\w+)((?:\[[^\]]*\])*)$', tail)
                    members.append ((synth, nm.group (1), re.findall (r'\[([^\]]*)\]', nm.group (2)), False))
                else:
                    members.extend (inner)
                i = j + 1 + k + 1 if k >= 0 else j + 1
            else:
                # A member with a brace initializer: "API_Guid guid {};"
                decl_members (head, members)
                i = j + 1 + k + 1 if k >= 0 else j + 1
            stmt = ''
            continue
        if c == ';':
            decl_members (stmt, members)
            stmt = ''
            i += 1
            continue
        if c == ':' and re.search (r'\b(public|private|protected)\s*$', stmt):
            stmt = ''
            i += 1
            continue
        stmt += c
        i += 1
    return members


def decl_members (stmt, members):
    decl = ' '.join (stmt.split ())
    if not decl or '(' in decl or re.match (r'^(static|typedef|using|friend|enum|template|public|private|protected|constexpr|virtual)\b', decl):
        return
    decl = decl.split ('=')[0].strip ()
    if re.search (r'(?<!:):(?!:)', decl):
        return  # bit field
    decl = re.sub (r'\b(const|volatile|mutable|struct|union)\b', ' ', decl)
    for part_i, part in enumerate (decl.split (',')):
        part = part.strip ()
        if part_i == 0:
            m = re.match (r'^(.*?)([\s\*&]+)(\w+)((?:\s*\[[^\]]*\])*)$', part)
            if not m:
                return
            base_type = ' '.join (m.group (1).split ())
            ptr = '*' in m.group (2) or '&' in m.group (2)
            name, arrays = m.group (3), m.group (4)
        else:
            m = re.match (r'^([\*&\s]*)(\w+)((?:\s*\[[^\]]*\])*)$', part)
            if not m:
                return
            ptr = '*' in m.group (1) or '&' in m.group (1)
            name, arrays = m.group (2), m.group (3)
        members.append ((base_type, name, re.findall (r'\[([^\]]*)\]', arrays), ptr))


def build_block (support):
    """The tables of one DevKit, as the C++ lines of a namespace ElementFields block."""
    structs.clear ()
    union_types.clear ()
    inc = support / 'Inc'
    texts = []
    for h in sorted (list (inc.glob ('*.h')) + list (inc.glob ('*.hpp'))):
        texts.append (strip_comments (h.read_text (encoding='utf-8', errors='replace')))
    allt = '\n'.join (texts)

    enums = set (re.findall (r'typedef\s+enum\s*\w*\s*(?::\s*[\w:]+\s*)?\{[^{}]*\}\s*(\w+)\s*;', allt))
    enums |= set (m.group (2) for m in re.finditer (r'\benum\s+(class\s+)?(\w+)\s*(?::\s*[\w:\s]+)?\{', allt))
    aliases = {}
    for m in re.finditer (r'\btypedef\s+([\w:\s]+?)\s+(\w+)\s*;', allt):
        aliases.setdefault (m.group (2), ' '.join (m.group (1).split ()))
    for m in re.finditer (r'\busing\s+(\w+)\s*=\s*([\w:\s]+?)\s*;', allt):
        aliases.setdefault (m.group (1), ' '.join (m.group (2).split ()))
    defines = {m.group (1): int (m.group (2)) for m in re.finditer (r'#define\s+(\w+)\s+\(?\s*(\d+)\s*\)?\s*$', allt, re.M)}
    for m in re.finditer (r'\b(?:constexpr|const)\s+[\w:]+\s+(\w+)\s*=\s*(\d+)\s*;', allt):
        defines.setdefault (m.group (1), int (m.group (2)))
    # Enum constants with an explicit value (array sizes such as API_NameLen = 32).
    for m in re.finditer (r'\b(\w+)\s*=\s*(\d+)\s*[,}]', allt):
        defines.setdefault (m.group (1), int (m.group (2)))

    for m in re.finditer (r'\bstruct\s+(API_\w+)\s*(?::[^{;]*)?\{', allt):
        j = find_matching (allt, m.end () - 1)
        if j > 0 and m.group (1) not in structs:
            structs[m.group (1)] = parse_members (allt[m.end ():j], m.group (1))
    for m in re.finditer (r'\btypedef\s+struct\s*\w*\s*\{', allt):
        j = find_matching (allt, m.end () - 1)
        nm = re.match (r'\s*(API_\w+)\s*;', allt[j + 1:j + 160])
        if nm and nm.group (1) not in structs:
            structs[nm.group (1)] = parse_members (allt[m.end ():j], nm.group (1))

    def resolve (t, seen=None):
        seen = seen or set ()
        t = t.strip ()
        while t in aliases and t not in seen and t not in structs and t not in enums:
            seen.add (t)
            t = aliases[t]
        return t

    u = allt.find ('union API_Element {')
    union_members = parse_members (allt[u + len ('union API_Element {'):find_matching (allt, u + len ('union API_Element '))], 'API_Element')
    elem_ids = set (re.findall (r'\b(API_\w+ID)\b', allt[allt.find ('API_ZombieElemID'):allt.find ('API_ZombieElemID') + 20000]))

    def array_size (spec):
        spec = spec.strip ()
        if spec.isdigit ():
            return int (spec)
        expr = re.sub (r'\b([A-Za-z_]\w*)\b', lambda mm: str (defines.get (mm.group (1), 'X')), spec)
        if 'X' in expr or not re.match (r'^[\d\s+\-*/()]+$', expr):
            return None
        try:
            return int (eval (expr, {'__builtins__': {}}))
        except Exception:
            return None

    def leaf_kind (t):
        r = resolve (t)
        if r in REAL or t in REAL:
            return 'Real'
        if r in BOOL or t in BOOL:
            return 'Bool'
        if r in SIGNED or t in SIGNED:
            return 'SignedInt'
        if r in UNSIGNED or t in UNSIGNED:
            return 'UnsignedInt'
        if t in enums or r in enums:
            return 'SignedInt'
        if t == 'API_AttributeIndex' or r == 'API_AttributeIndex':
            return 'AttributeIndex'
        if t == 'API_Guid' or r == 'API_Guid':
            return 'Guid'
        return None

    # variant: the member of a named union the field is in ("u.revolvedShell"), or None. The members of
    # a union share their bytes, so only the one the element uses holds its values.
    def expand (typ, path, depth, rows, variant=None):
        in_union = typ in union_types
        for mtype, mname, arrays, ptr in structs.get (typ, []):
            if ptr or re.match (r'^(filler|reserved|dummy|unused|head$)', mname, re.I) or 'GS::' in mtype and mtype not in UCHARS | SIGNED | UNSIGNED | REAL:
                continue
            p = f'{path}.{mname}' if path else mname
            v = p if in_union else variant
            r = resolve (mtype)
            if arrays:
                if len (arrays) == 1 and (r in CHARS or mtype in CHARS):
                    rows.append ((p, 'CString', mname, v))
                    continue
                if len (arrays) == 1 and (mtype in UCHARS or r == 'GS::uchar_t') and mtype != 'unsigned short':
                    rows.append ((p, 'UString', mname, v))
                    continue
                if len (arrays) != 1:
                    continue
                n = array_size (arrays[0])
                if n is None or n > MAX_ARRAY:
                    continue
                for k in range (n):
                    q = f'{p}[{k}]'
                    kind = leaf_kind (mtype)
                    if kind and kind != 'Guid':
                        rows.append ((q, kind, mname, v))
                    elif (mtype in structs or r in structs) and depth < MAX_DEPTH:
                        expand (mtype if mtype in structs else r, q, depth + 1, rows, v)
                continue
            kind = leaf_kind (mtype)
            if kind:
                rows.append ((p, kind, mname, v))
            elif (mtype in structs or r in structs) and depth < MAX_DEPTH:
                expand (mtype if mtype in structs else r, p, depth + 1, rows, v)

    def ancestors (path):
        """The paths of the members a field is in, outermost first: "a.b[0].c" -> ["a", "a.b", "a.b[0]"]."""
        out = []
        for m in re.finditer (r'\.|\[', path):
            out.append (path[:m.start ()])
        return out

    tables = []
    for etype, emember, _, _ in union_members:
        if emember == 'header':
            continue
        st = etype if etype in structs else resolve (etype)
        if st not in structs:
            continue
        eid = ELEM_ID_EXCEPTIONS.get (emember, 'API_' + emember[0].upper () + emember[1:] + 'ID')
        if eid not in elem_ids:
            print ('no element type id for', emember, eid, file=sys.stderr)
            continue
        rows = []
        expand (st, '', 0, rows)
        if rows:
            tables.append ((eid, etype, emember, rows))

    # The fields that choose which member of a union, or which shape, an element has. Changing one
    # without the data of the other shape breaks the element: Archicad 29 quit when a roof's
    # roofClass was changed through SetElementFields, and a wall's type is issue #1914.
    shape_selectors = {
        ('API_RoofType', 'roofClass'), ('API_ShellType', 'shellClass'), ('API_LabelType', 'labelClass'),
        ('API_StairStructureType', 'structType'), ('API_WallType', 'type'),
    }

    def read_only (path, kind, leaf, struct):
        # Owners and counts are computed by Archicad; other guids (e.g. symbID, the library part of a
        # railing component) are settings and stay writable.
        return (leaf in ('owner', 'guid', 'ownerGuid') or re.match (r'^n[A-Z]', leaf) is not None or
                (struct, path) in shape_selectors)

    lines = [
        'namespace ElementFields {',
        '',
    ]
    total = 0
    max_anc = 0
    for eid, etype, emember, rows in tables:
        lines.append (f'static const FieldInfo Fields_{eid}[] = {{')
        for p, kind, leaf, variant in rows:
            anc = ancestors (p)
            max_anc = max (max_anc, len (anc))
            anc_code = ', '.join (f'TF_OFF ({etype}, {a})' for a in anc)
            var_code = f'"{variant}"' if variant else 'nullptr'
            lines.append (f'    {{ "{p}", TF_OFF ({etype}, {p}), TF_SIZE ({etype}, {p}), FieldKind::{kind}, {"true" if read_only (p, kind, leaf, etype) else "false"}, '
                          f'{var_code}, {len (anc)}, {{ {anc_code} }} }},')
        lines.append ('};')
        lines.append ('')
        total += len (rows)
    if max_anc > 12:
        sys.exit (f'A field has {max_anc} enclosing members; FieldInfo::ancestors holds 12.')
    lines.append ('const FieldTable* GetFieldTable (API_ElemTypeID typeID)')
    lines.append ('{')
    lines.append ('    switch (typeID) {')
    for eid, etype, emember, rows in tables:
        lines.append (f'        case {eid}: {{ static const FieldTable table = {{ "{etype}", "{emember}", Fields_{eid}, static_cast<UInt32> (sizeof (Fields_{eid}) / sizeof (FieldInfo)) }}; return &table; }}')
    lines.append ('        default: return nullptr;')
    lines.append ('    }')
    lines.append ('}')
    lines.append ('')
    lines.append ('bool IsAvailable ()')
    lines.append ('{')
    lines.append ('    return true;')
    lines.append ('}')
    lines.append ('')
    lines.append ('}')
    return lines, len (tables), total


def main ():
    """generate_element_field_table.py VERSION=SUPPORT_DIR [VERSION=SUPPORT_DIR ...] OUT.cpp"""
    devkits = []
    for arg in sys.argv[1:-1]:
        version, _, support = arg.partition ('=')
        devkits.append ((int (version), pathlib.Path (support)))
    out = pathlib.Path (sys.argv[-1])
    lines = [
        '// Generated by tools/generate_element_field_table.py from the headers of the DevKits',
        '// ' + ', '.join (f'{s.parent.name}' for _, s in devkits) + '. Do not edit: run the generator again.',
        '// One table per Archicad version; other versions get none.',
        '',
        '#include "ElementFieldTable.hpp"',
        '',
        '#include <cstddef>',
        '',
        '#if defined (__clang__)',
        '#pragma clang diagnostic ignored "-Winvalid-offsetof"',
        '#endif',
        '',
        '#define TF_OFF(T, P) static_cast<UInt32> (offsetof (T, P))',
        '#define TF_SIZE(T, P) static_cast<UInt32> (sizeof (((T*) nullptr)->P))',
        '',
    ]
    for i, (version, support) in enumerate (sorted (devkits)):
        block, ntables, total = build_block (support)
        keyword = '#if' if i == 0 else '#elif'
        lines.append (f'{keyword} defined (ServerMainVers_{version}00) && !defined (ServerMainVers_{version + 1}00)')
        lines.append (f'// Archicad {version}, from {support.parent.name}')
        lines.append ('')
        lines += block
        lines.append ('')
        print (f'Archicad {version}: {ntables} element types, {total} fields')
    lines += [
        '#else',
        '',
        'namespace ElementFields {',
        '',
        'const FieldTable* GetFieldTable (API_ElemTypeID /*typeID*/)',
        '{',
        '    return nullptr;',
        '}',
        '',
        'bool IsAvailable ()',
        '{',
        '    return false;',
        '}',
        '',
        '}',
        '',
        '#endif',
    ]
    out.write_text ('\n'.join (lines) + '\n', encoding='utf-8')
    print ('->', out)


if __name__ == '__main__':
    main ()
