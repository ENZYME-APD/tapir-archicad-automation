"""Generate the reference of the Tapir Grasshopper plugin's components (#16).

Reads the component classes of grasshopper-plugin/TapirGrasshopperPlugin/Components
and writes docs/grasshopper-components/components.js, the data of the page
docs/grasshopper-components/index.html: for each component its name, group,
description, inputs and outputs (name, kind and description), the Tapir commands
it calls (linked to the command reference) and, for the Archicad tool components
(ElementToolComponent), the keys of each JSON input from ToolSettings/<Tool>.json.

It reads the C# sources, it does not build the plugin, so it knows what the code
spells out: the In*/Out* calls of AddInputs and AddOutputs (in the class and its
base classes, through base.AddInputs () and helper methods), the Field lists and
InputDescriptors of the Create/Modify element components, and the panels
(JsonGroups) of the tool components. A name or description it cannot work out
(built at run time) is left out rather than guessed, and the component is still
listed. Standard library only, like the other generators:

    python tools/generate_gh_docs.py           # write the data file
    python tools/generate_gh_docs.py --check   # exit 1 when it is out of date
"""

import argparse
import json
import pathlib
import re
import sys

sys.path.insert (0, str (pathlib.Path (__file__).resolve ().parent))
import generate_gh_coverage as coverage  # noqa: E402

ROOT = pathlib.Path (__file__).resolve ().parent.parent
PLUGIN = ROOT / 'grasshopper-plugin' / 'TapirGrasshopperPlugin'
COMPONENTS = PLUGIN / 'Components'
TOOL_SETTINGS = PLUGIN / 'ToolSettings'
COMMAND_DEFINITIONS = ROOT / 'docs' / 'archicad-addon' / 'command_definitions.js'
OUT = ROOT / 'docs' / 'grasshopper-components' / 'components.js'
SOURCE_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/grasshopper-plugin/TapirGrasshopperPlugin/'

# The helpers of Component that add a parameter: (direction, kind) from the method name.
PARAM_RE = re.compile (r'^(In|Out)(Generic|Vector|Point|Number|Boolean|Integer|Text|Color|Box|Curve|Mesh|Line)'
                       r'(es|s|Tree|Item|List|WithDefault)?$')
KIND_NAMES = {
    'Generic': 'generic', 'Vector': 'vector', 'Point': 'point', 'Number': 'number', 'Boolean': 'boolean',
    'Integer': 'integer', 'Text': 'text', 'Color': 'color', 'Box': 'box', 'Curve': 'curve', 'Mesh': 'mesh',
    'Line': 'line',
}
ACCESS_NAMES = {None: 'item', 'Item': 'item', 'WithDefault': 'item', 's': 'list', 'es': 'list', 'List': 'list', 'Tree': 'tree'}
# The kinds of the Field of CreateElementsComponentBase / ModifyElementsComponentBase.
FIELD_KINDS = {
    'Number': 'number', 'Integer': 'integer', 'Boolean': 'boolean', 'Text': 'text', 'Point2D': 'point',
    'Point3D': 'point', 'Line': 'line', 'ElementGuid': 'generic', 'AttributeGuid': 'generic',
    'PointsTree2D': 'point tree', 'PointsTree3D': 'point tree', 'OutlineCurve': 'curve',
    'HoleCurvesTree': 'curve tree', 'Color': 'color', 'AdditionalSettings': 'text list',
    'NumbersTree': 'number tree', 'IntegersTree': 'integer tree', 'DatabaseId': 'generic',
    'BaseLineCurve': 'curve',
}
IDENT = r'[A-Za-z_][A-Za-z0-9_]*'
MAX_DEPTH = 8


class Unknown (Exception):
    """An expression whose value only exists at run time."""


# ------------------------------------------------------------- C# syntax --

def matching (text, start, open_char='(', close_char=')'):
    """The index after the bracket that closes the one at start; strings are skipped."""
    depth, i, n = 0, start, len (text)
    while i < n:
        c = text[i]
        if c == '"' or (c in '@$' and text[i + 1:i + 2] == '"') or (c in '@$' and text[i + 1:i + 3] in ('@"', '$"')):
            i = string_end (text, i)
            continue
        if c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                j += 2 if text[j] == '\\' else 1
            i = j + 1
            continue
        if c == open_char:
            depth += 1
        elif c == close_char:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return n


def string_end (text, i):
    """The index after the string literal that starts at i ("...", @"...", $"...", $@"...")."""
    prefix = ''
    while text[i] in '@$':
        prefix += text[i]
        i += 1
    verbatim = '@' in prefix
    j = i + 1
    n = len (text)
    while j < n:
        if text[j] == '\\' and not verbatim:
            j += 2
            continue
        if text[j] == '"':
            if verbatim and text[j + 1:j + 2] == '"':
                j += 2
                continue
            return j + 1
        j += 1
    return n


def split_args (text):
    """The top-level, comma-separated parts of an argument list (without its parentheses)."""
    parts, depth, start, i, n = [], 0, 0, 0, len (text)
    while i < n:
        c = text[i]
        if c == '"' or (c in '@$' and '"' in text[i + 1:i + 3]):
            i = string_end (text, i)
            continue
        if c in '([{<' and not (c == '<' and not re.match (r'<\s*[A-Z]', text[i:i + 3])):
            depth += 1
        elif c in ')]}>' and depth > 0 and not (c == '>' and text[i - 1:i] == '='):
            depth -= 1
        elif c == ',' and depth == 0:
            parts.append (text[start:i].strip ())
            start = i + 1
        i += 1
    last = text[start:].strip ()
    if last or parts:
        parts.append (last)
    return parts


def unescape (body, verbatim):
    if verbatim:
        return body.replace ('""', '"')
    return re.sub (r'\\(u[0-9a-fA-F]{4}|.)', lambda m: chr (int (m.group (1)[1:], 16)) if m.group (1)[0] == 'u'
                   else {'n': '\n', 't': '\t', '"': '"', '\\': '\\', "'": "'"}.get (m.group (1), m.group (1)), body)


# ------------------------------------------------------------ the plugin --

class Plugin:
    def __init__ (self, root):
        self.classes = coverage.read_plugin (root)
        self.group_names = self.read_group_names ()

    def read_group_names (self):
        names = {}
        info = self.classes.get ('GroupNames')
        if info:
            for m in re.finditer (r'\b(' + IDENT + r')\s*=>\s*(?:nameof\s*\(\s*(' + IDENT + r')\s*\)|"([^"]*)")', info['text']):
                names[m.group (1)] = m.group (2) or m.group (3)
        return names

    def chain (self, name):
        return coverage.chain (self.classes, name)

    def member (self, owner, name):
        """The text of the expression-bodied member or initializer 'name' (=> ... ; or { get; } = ... ;),
        nearest in owner's chain."""
        pattern = re.compile (r'\b' + re.escape (name) + r'\s*(?:\{\s*get;\s*\}\s*=(?!=)|=>)')
        for cls in self.chain (owner):
            text = self.classes[cls]['text']
            for m in pattern.finditer (text):
                start = m.end ()
                return text[start:statement_end (text, start)].strip ()
        return None

    def field (self, owner, name):
        """The initializer of a field 'name' (static readonly List<Field> name = ...;), nearest in the chain."""
        pattern = re.compile (r'\b' + re.escape (name) + r'\s*=(?![=>])')
        for cls in self.chain (owner):
            text = self.classes[cls]['text']
            for m in pattern.finditer (text):
                if re.search (r'\b(?:var|this\.)\s*$', text[max (0, m.start () - 8):m.start ()]):
                    continue
                start = m.end ()
                return text[start:statement_end (text, start)].strip ()
        return None

    def method (self, owner, name, start_after=None):
        """(class, body) of the nearest method 'name' in owner's chain, below start_after when given."""
        names = self.chain (owner)
        if start_after is not None:
            names = names[names.index (start_after) + 1:] if start_after in names else []
        pattern = re.compile (r'\bvoid\s+' + re.escape (name) + r'\s*\([^)]*\)\s*\{|\b' + re.escape (name) + r'\s*\{\s*get\s*\{')
        for cls in names:
            text = self.classes[cls]['text']
            m = pattern.search (text)
            if m:
                return cls, text[m.end ():matching (text, m.end () - 1, '{', '}') - 1]
        return None, None


def statement_end (text, start):
    """The index of the ; that ends the statement starting at start (brackets and strings skipped)."""
    i, n = start, len (text)
    while i < n:
        c = text[i]
        if c == '"' or (c in '@$' and '"' in text[i + 1:i + 3]):
            i = string_end (text, i)
            continue
        if c in '([{':
            i = matching (text, i, c, {'(': ')', '[': ']', '{': '}'}[c])
            continue
        if c == ';':
            return i
        i += 1
    return n


class Evaluator:
    """Works out the strings, booleans and string lists of C# expressions from the literals in the
    sources, the constructor arguments and the expression-bodied members of a component's chain."""

    def __init__ (self, plugin, owner, env=None):
        self.plugin = plugin
        self.owner = owner
        self.env = env or {}

    def with_env (self, **values):
        env = dict (self.env)
        env.update (values)
        return Evaluator (self.plugin, self.owner, env)

    def value (self, expr, depth=0):
        expr = expr.strip ()
        if depth > MAX_DEPTH or not expr:
            raise Unknown (expr)
        while expr.startswith ('(') and matching (expr, 0) == len (expr):
            expr = expr[1:-1].strip ()
        parts = self.concatenation (expr)
        if len (parts) > 1:
            values = [self.value (p, depth + 1) for p in parts]
            if all (isinstance (v, str) for v in values):
                return ''.join (values)
            raise Unknown (expr)
        if expr[0] == '"' or expr[:2] in ('@"', '$"') or expr[:3] in ('$@"', '@$"'):
            if string_end (expr, 0) != len (expr):
                raise Unknown (expr)
            prefix = expr[:expr.index ('"')]
            body = unescape (expr[len (prefix) + 1:-1], '@' in prefix)
            if '$' in prefix:
                return re.sub (r'\{([^{}]+)\}', lambda m: str (self.value (m.group (1), depth + 1)),
                               body.replace ('{{', '\x01').replace ('}}', '\x02')).replace ('\x01', '{').replace ('\x02', '}')
            return body
        if expr in ('true', 'false'):
            return expr == 'true'
        if expr == 'null':
            return None
        m = re.fullmatch (r'nameof\s*\(\s*(?:' + IDENT + r'\.)*(' + IDENT + r')\s*\)', expr)
        if m:
            return m.group (1)
        m = re.fullmatch (r'GroupNames\.(' + IDENT + r')', expr)
        if m and m.group (1) in self.plugin.group_names:
            return self.plugin.group_names[m.group (1)]
        m = re.fullmatch (r'(' + IDENT + r')\s*\?\?\s*(.+)', expr, re.S)
        if m:
            first = self.value (m.group (1), depth + 1)
            return first if first is not None else self.value (m.group (2), depth + 1)
        m = re.fullmatch (r'(.+?)\.WithTypeName\s*\(.*\)', expr, re.S)
        if m:
            return self.value (m.group (1), depth + 1)
        m = re.fullmatch (r'new\s*(?:[A-Za-z_<>]+\s*)?\[\s*\]\s*(\{.*\})|new\s+(?:List|HashSet)<string>\s*(?:\(\s*\))?\s*(\{.*\})', expr, re.S)
        if m:
            items = (m.group (1) or m.group (2))[1:-1]
            return [self.value (p, depth + 1) for p in split_args (items) if p]
        if expr == 'Table.Tool':
            return self.tool_name ()
        m = re.fullmatch (r'(' + IDENT + r')\.(' + IDENT + r')', expr)
        if m and m.group (1) in self.plugin.classes:
            member = self.plugin.member (m.group (1), m.group (2))
            if member is None:
                member = self.plugin.field (m.group (1), m.group (2))
            if member is not None:
                return Evaluator (self.plugin, m.group (1)).value (member, depth + 1)
        if re.fullmatch (IDENT, expr):
            if expr in self.env:
                return self.env[expr]
            member = self.plugin.member (self.owner, expr)
            if member is not None:
                return self.value (member, depth + 1)
        m = re.fullmatch (r'GroupParamName\s*\((.+)\)', expr, re.S)
        if m:
            return group_param_name (self.value (m.group (1), depth + 1))
        raise Unknown (expr)

    def text (self, expr):
        """The string value of expr, or None when it is only known at run time."""
        try:
            value = self.value (expr)
        except (Unknown, RecursionError, ValueError, IndexError):
            return None
        return value if isinstance (value, str) else None

    def tool_name (self):
        name = self.value ('ToolName')
        table = read_tool_settings (name)
        return table.get ('tool', name) if table else name

    @staticmethod
    def concatenation (expr):
        """The operands of a top-level + chain."""
        parts, depth, start, i, n = [], 0, 0, 0, len (expr)
        while i < n:
            c = expr[i]
            if c == '"' or (c in '@$' and '"' in expr[i + 1:i + 3]):
                i = string_end (expr, i)
                continue
            if c in '([{':
                depth += 1
            elif c in ')]}':
                depth -= 1
            elif c == '+' and depth == 0:
                parts.append (expr[start:i])
                start = i + 1
            i += 1
        parts.append (expr[start:])
        return [p.strip () for p in parts]


def group_param_name (group):
    """ElementToolComponent.GroupParamName: "Floor Plan and Section" -> FloorPlanAndSection_JSON."""
    words = [''.join (ch for ch in w if ch.isalnum ()) for w in group.split ()]
    return ''.join (w[0].upper () + w[1:] for w in words if w) + '_JSON'


TOOL_TABLES = {}


def read_tool_settings (name):
    if name not in TOOL_TABLES:
        path = TOOL_SETTINGS / f'{name}.json'
        TOOL_TABLES[name] = json.loads (path.read_text (encoding='utf-8')) if path.is_file () else None
    return TOOL_TABLES[name]


# ------------------------------------------------------------ components --

def constructor_values (plugin, name):
    """[name, description, group] of a component, worked out through the constructors of its chain
    (each one's base (...) call evaluated with the arguments the class below it passed)."""
    values = None
    for cls in plugin.chain (name):
        text = plugin.classes[cls]['text']
        ctors = []
        for m in re.finditer (r'\b(?:public|protected|internal|private)\s+' + re.escape (cls) + r'\s*\(', text):
            params_end = matching (text, m.end () - 1)
            params = [p for p in split_args (text[m.end ():params_end - 1]) if p]
            rest = text[params_end:]
            b = re.match (r'\s*:\s*base\s*\(', rest)
            args = split_args (rest[b.end ():matching (rest, b.end () - 1) - 1]) if b else None
            ctors.append ((params, args))
        if not ctors:
            continue
        if values is None:
            chosen = next ((c for c in ctors if not c[0]), ctors[0])
            env = {}
        else:
            chosen = next ((c for c in ctors if len (c[0]) == len (values)), None)
            if chosen is None:
                chosen = next ((c for c in ctors if len (c[0]) >= len (values) and all ('=' in p for p in c[0][len (values):])), None)
            if chosen is None:
                return values
            env = {}
            for index, param in enumerate (chosen[0]):
                pname = re.split (r'\s*=\s*', param)[0].split ()[-1]
                if index < len (values):
                    env[pname] = values[index]
                elif '=' in param:
                    try:
                        env[pname] = Evaluator (plugin, name).value (param.split ('=', 1)[1])
                    except Unknown:
                        env[pname] = None
        if cls == 'Component' or chosen[1] is None:
            return [env.get (p.split ()[-1]) for p in chosen[0]] if cls == 'Component' else values
        evaluator = Evaluator (plugin, name, env)
        values = [evaluator.text (a) if not re.fullmatch (r'(true|false|\d+)', a) else a for a in chosen[1]]
    return values


class Collector:
    """The inputs and outputs a component registers, in their order."""

    def __init__ (self, plugin, owner):
        self.plugin = plugin
        self.owner = owner
        self.params = {'In': [], 'Out': []}
        self.registering = 'In'
        self.incomplete = set ()

    def add (self, direction, name, kind, description):
        if name is None:
            return
        self.params[direction].append ({'name': name, 'kind': kind, 'description': description or ''})

    def run (self, method, evaluator, after=None, depth=0):
        cls, body = self.plugin.method (self.owner, method, after)
        if body is not None and depth < MAX_DEPTH:
            self.statements (body, cls, method, evaluator, depth)

    def statements (self, body, cls, method, evaluator, depth):
        i, n = 0, len (body)
        while i < n:
            m = re.compile (r'\b(foreach|for|if|else|base\s*\.\s*' + IDENT + r'|new\s+InputDescriptor|(?:inManager|outManager|pManager)\s*\.\s*Add' + IDENT + r'|' + IDENT + r')\s*\(|\belse\b').search (body, i)
            if not m:
                break
            word = re.sub (r'\s+', '', m.group (1) or 'else')
            start = m.end () - 1 if m.group (1) else m.end ()
            if word == 'else':
                i = m.end ()
                continue
            end = matching (body, start) if m.group (1) else start
            head = body[start + 1:end - 1] if m.group (1) else ''
            if word in ('foreach', 'for', 'if'):
                block_start = end
                while block_start < n and body[block_start].isspace ():
                    block_start += 1
                if block_start < n and body[block_start] == '{':
                    block_end = matching (body, block_start, '{', '}')
                    block = body[block_start + 1:block_end - 1]
                else:
                    block_end = statement_end (body, block_start) + 1
                    block = body[block_start:block_end]
                if word == 'foreach':
                    self.foreach (head, block, cls, method, evaluator, depth)
                elif word == 'for':
                    # for (var index = 0; index < fields.Count; index++) over a local copy of Fields.
                    count = re.search (r'<\s*(' + IDENT + r')\.Count', head)
                    local = count and re.search (r'\b(?:var|IReadOnlyList<Field>)\s+' + re.escape (count.group (1)) +
                                                 r'\s*=\s*(Fields|TrailingFields)\s*;', body)
                    if local:
                        for field in self.fields (local.group (1)):
                            self.add ('In', field['name'], field['kind'], field['description'])
                    elif re.search (r'\b(?:In|Out|Add)[A-Z]\w*\s*\(', block):
                        self.incomplete.add (self.registering)
                elif word == 'if':
                    try:
                        condition = evaluator.value (head)
                    except Unknown:
                        condition = True
                    if condition is not False:
                        self.statements (block, cls, method, evaluator, depth + 1)
                i = block_end
                continue
            args = split_args (head)
            consumed = True
            if word.startswith ('base.'):
                if word[5:] == method:
                    self.run (method, evaluator, after=cls, depth=depth + 1)
            elif word == 'newInputDescriptor':
                self.input_descriptor (args, evaluator)
            elif (re.fullmatch (r'Add(' + IDENT + r')?(Input|Output)', word) or re.fullmatch (r'AddOptional' + IDENT, word)) and not word.startswith ('AddAs'):
                self.helper_param (word, args, evaluator)
            else:
                qualifier = re.search (r'(' + IDENT + r')\s*\.\s*$', body[max (0, m.start () - 80):m.start ()])
                qualifier = qualifier.group (1) if qualifier else None
                if qualifier and qualifier in self.plugin.classes and qualifier != self.owner:
                    owner_cls, helper = self.static_method (qualifier, word)
                    if helper is not None and depth < MAX_DEPTH:
                        self.statements (helper, owner_cls, word, evaluator, depth + 1)
                elif qualifier in (None, 'this', 'component', 'Component'):
                    consumed = self.call (word, args, evaluator, depth)
                else:
                    consumed = False
            # A call that adds no parameter itself (descriptors.Add (...), a lambda) may hold one in its arguments.
            i = end if consumed else start + 1
        return

    def call (self, word, args, evaluator, depth):
        manager = re.match (r'(inManager|outManager|pManager)\.Add(' + IDENT + r')Parameter$', word)
        if manager:
            if len (args) >= 3:
                direction = 'Out' if manager.group (1) == 'outManager' or (manager.group (1) == 'pManager' and self.registering == 'Out') else 'In'
                kind = re.sub (r'(?<!^)(?=[A-Z])', ' ', manager.group (2)).lower ()
                access = re.search (r'GH_ParamAccess\.(\w+)', args[3]) if len (args) > 3 else None
                if access and access.group (1) != 'item':
                    kind += ' ' + access.group (1)
                self.add (direction, evaluator.text (args[0]), kind, evaluator.text (args[2]))
            return True
        if word == 'OutErrorMessages':
            self.add ('Out', 'ErrorMessages', 'text list', evaluator.text (args[0]) if args and args[0] else
                      'Error message of each item (empty when the operation succeeded on it).')
            return True
        m = PARAM_RE.match (word)
        if m and args:
            kind = KIND_NAMES[m.group (2)] + ('' if ACCESS_NAMES[m.group (3)] == 'item' else ' ' + ACCESS_NAMES[m.group (3)])
            description = evaluator.text (args[1]) if len (args) > 1 else ''
            self.add (m.group (1), evaluator.text (args[0]), kind, description)
            return True
        if word in ('SetOptionality', 'AddAsSource', 'nameof'):
            return True
        if word[0].islower () or word == 'Add':
            return False
        cls, body = self.plugin.method (self.owner, word)
        if body is not None and depth < MAX_DEPTH:
            self.statements (body, cls, word, evaluator, depth + 1)
            return True
        return False

    def helper_param (self, word, args, evaluator):
        """A parameter added by a helper such as this.AddNumberOutput ("Name", "Description") or
        NavigatorComponentHelp.AddOutput (component, new Param_Number (), "Name", "Description"): the
        first two arguments that are texts are its name and description."""
        m = re.fullmatch (r'Add(' + IDENT + r')?(Input|Output)', word) or re.fullmatch (r'Add(Optional' + IDENT + r')()', word)
        texts = [t for t in (evaluator.text (a) for a in args) if t is not None]
        if not texts:
            return
        param = next ((re.search (r'new\s+Param_(' + IDENT + r')', a) for a in args if 'Param_' in a), None)
        kind = (param.group (1) if param else (m.group (1) or '')).replace ('Optional', '').replace ('Item', '')
        kind = re.sub (r'(?<!^)(?=[A-Z])', ' ', kind).lower ()
        self.add ('Out' if m.group (2) == 'Output' else 'In', texts[0], kind.strip (), texts[1] if len (texts) > 1 else '')

    def static_method (self, cls, name):
        text = self.plugin.classes[cls]['text']
        m = re.search (r'\bvoid\s+' + re.escape (name) + r'\s*\([^)]*\)\s*\{', text)
        if not m:
            return None, None
        return cls, text[m.end ():matching (text, m.end () - 1, '{', '}') - 1]

    def foreach (self, head, block, cls, method, evaluator, depth):
        m = re.match (r'\s*(?:var|string|' + IDENT + r')\s+(' + IDENT + r')\s+in\s+(.+)$', head, re.S)
        if not m:
            return
        variable, source = m.group (1), m.group (2).strip ()
        if source == 'InputDescriptors':
            return
        if source in ('Fields', 'TrailingFields'):
            for field in self.fields (source):
                self.add ('In', field['name'], field['kind'], field['description'])
            return
        try:
            items = evaluator.value (source)
        except Unknown:
            if re.search (r'\b(?:In|Out|Add)[A-Z]\w*\s*\(', block):
                self.incomplete.add (self.registering)
            return
        if isinstance (items, list):
            for item in items:
                self.statements (block, cls, method, evaluator.with_env (**{variable: item}), depth + 1)

    def input_descriptor (self, args, evaluator):
        if not args:
            return
        name = evaluator.text (args[0])
        description = ''
        call = re.search (r'(' + IDENT + r')\s*\((.*)\)\s*$', args[1] if len (args) > 1 else '', re.S)
        if call:
            inner = split_args (call.group (2))
            texts = [evaluator.text (a) for a in inner]
            if name in texts:
                after = [t for t in texts[texts.index (name) + 1:] if t is not None]
                description = after[0] if after else ''
        self.add ('In', name, 'boolean' if 'Boolean' in (call.group (1) if call else '') else 'text list'
                  if name == 'AdditionalSettings' else '', description)

    def fields (self, member):
        """The Field items of the Fields or TrailingFields list of the component."""
        expr = self.plugin.member (self.owner, member)
        if expr is None:
            return []
        expr = expr.strip ()
        if re.fullmatch (IDENT, expr):
            expr = self.plugin.field (self.owner, expr) or ''
        found = []
        evaluator = Evaluator (self.plugin, self.owner)
        for m in re.finditer (r'new\s+Field\s*\(', expr):
            args = split_args (expr[m.end ():matching (expr, m.end () - 1) - 1])
            if len (args) < 4:
                continue
            kind = re.sub (r'^FieldKind\.', '', args[2])
            found.append ({'name': evaluator.text (args[0]), 'kind': FIELD_KINDS.get (kind, ''),
                           'description': evaluator.text (args[3]) or ''})
        return found


def tool_json_groups (plugin, name):
    """[{name: <panel>, input: <Group>_JSON, keys: [{key, writable}]}] of a tool component."""
    if 'ElementToolComponent' not in plugin.chain (name):
        return None
    evaluator = Evaluator (plugin, name)
    try:
        tool = evaluator.value ('ToolName')
        groups = evaluator.value ('JsonGroups')
    except Unknown:
        return None
    table = read_tool_settings (tool) or {}
    try:
        use_labels = evaluator.value ('UseLabels') is not False
    except Unknown:
        use_labels = True
    result = []
    for group in groups if isinstance (groups, list) else []:
        keys = []
        for field in table.get ('fields', []):
            if field.get ('group') != group:
                continue
            api = field.get ('get') or field.get ('set')
            label = field.get ('label') if use_labels else None
            keys.append ({'key': label or api, 'api': api, 'writable': bool (field.get ('set')),
                          'subgroup': field.get ('subgroup')})
        result.append ({'name': group, 'input': group_param_name (group), 'keys': keys})
    return result


def command_names ():
    """Command name -> group name, from the command reference's data."""
    text = COMMAND_DEFINITIONS.read_text (encoding='utf-8')
    data = json.loads (text[text.index ('['):text.rindex (']') + 1])
    return {command['name']: group['name'] for group in data for command in group['commands']}


def build ():
    plugin = Plugin (PLUGIN)
    classes = plugin.classes
    commands = command_names ()
    used = coverage.used_commands (classes, set (commands))
    by_component = {}
    for command, owners in used.items ():
        for component in coverage.components_of (classes, owners):
            by_component.setdefault (component, set ()).add (command)

    components = []
    for name, info in sorted (classes.items ()):
        names = plugin.chain (name)
        if info['abstract'] or 'Component' not in names[1:] or 'ComponentGuid' not in ''.join (classes[c]['text'] for c in names):
            continue
        values = constructor_values (plugin, name) or []
        display = values[0] if values else None
        evaluator = Evaluator (plugin, name)
        exposure = plugin.member (name, 'Exposure') or ''
        collector = Collector (plugin, name)
        uses_descriptors = 'ToggleableInputsComponentBase' in names
        if uses_descriptors:
            collector.run ('InputDescriptors', evaluator)
        collector.run ('AddInputs', evaluator)
        collector.registering = 'Out'
        collector.run ('AddOutputs', evaluator)
        component = {
            'name': display or name,
            'className': name,
            'group': values[2] if len (values) > 2 and values[2] else None,
            'description': values[1] if len (values) > 1 and values[1] else '',
            'commands': sorted (by_component.get (name, ())),
            'inputs': collector.params['In'],
            'outputs': collector.params['Out'],
            'source': SOURCE_URL + info['path'],
        }
        for direction, key in (('In', 'inputsIncomplete'), ('Out', 'outputsIncomplete')):
            if direction in collector.incomplete:
                component[key] = True
        if 'GH_Exposure.hidden' in exposure:
            component['hidden'] = True
        json_groups = tool_json_groups (plugin, name)
        if json_groups is not None:
            component['jsonGroups'] = json_groups
        components.append (component)

    group_order = sorted ({c['group'] or 'Other' for c in components}, key=lambda g: (g == 'Other', g.lower ()))
    return sorted (components, key=lambda c: (group_order.index (c['group'] or 'Other'), c['name'].lower (), c['className']))


def render (components):
    """The data file: one component per line, so a change to one shows as one line in a diff."""
    lines = [json.dumps (c, ensure_ascii=False, separators=(',', ':')) for c in components]
    return ('// Generated by tools/generate_gh_docs.py from the Grasshopper plugin\'s sources. Do not edit by hand.\n'
            'var gGhComponents = [\n' + ',\n'.join (lines) + '\n];\n')


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--check', action='store_true', help='exit with 1 when the data file is out of date, write nothing')
    parser.add_argument ('--stats', action='store_true', help='also print how many components have no inputs or outputs found')
    args = parser.parse_args ()

    components = build ()
    content = render (components)
    if args.stats:
        print (f"{len (components)} components in {len ({c['group'] for c in components})} groups")
        print ('without inputs: ' + ', '.join (c['className'] for c in components if not c['inputs']))
        print ('without outputs: ' + ', '.join (c['className'] for c in components if not c['outputs']))
        print ('without description: ' + ', '.join (c['className'] for c in components if not c['description']))
        print ('without group: ' + ', '.join (c['className'] for c in components if not c['group']))
    current = OUT.read_text (encoding='utf-8') if OUT.is_file () else None
    if current == content:
        print ('docs/grasshopper-components is up to date.')
        return 0
    if args.check:
        print ('docs/grasshopper-components is out of date. Run python tools/generate_gh_docs.py')
        return 1
    OUT.parent.mkdir (parents=True, exist_ok=True)
    OUT.write_text (content, encoding='utf-8', newline='\n')
    print ('Wrote docs/grasshopper-components/components.js')
    return 0


if __name__ == '__main__':
    sys.exit (main ())
