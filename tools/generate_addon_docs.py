"""Regenerates docs/archicad-addon from the Add-On sources, without Archicad.

Produces the same command_definitions.js and common_schema_definitions.js as
the GenerateDocumentation developer command (DeveloperTools.cpp), by reading
what that command reads at runtime straight from the C++ sources:

  * the command groups and RegisterCommand<> calls (version, description) in
    the Initialize function of AddOnMain.cpp,
  * each command's GetName, GetInputParametersSchema and GetRawResponseSchema,
    following the class hierarchy like the virtual calls do,
  * the common schema definitions resource (CommonSchemaDefinitions.json).

Schema functions are evaluated by a small interpreter covering the subset of
C++ the schemas are written in: string literals (raw and escaped), named
string constants, GS::UniString (...), GS::UniString::Printf (%s/%T), string
concatenation with + and +=, local string variables, if/else on bool
parameters, calls to static helper functions and to other commands' schema
methods, and returning {} / GS::NoValue for a missing schema. Anything else
stops the script with an error naming the function, so an unsupported
construct can never produce silently wrong docs - extend the interpreter (or
simplify the schema code) when that happens. Preprocessor conditions on
ServerMainVers_NNNN / AC_VERSION are resolved for the newest supported
Archicad version (the newest one build_all_win.bat builds), the one the
docs are generated with.

Usage: python tools/generate_addon_docs.py [--check]
  --check  do not write, exit with 1 when the docs are out of date
"""

import argparse
import glob
import json
import os
import re
import sys

REPO_ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
SOURCES_DIR = os.path.join (REPO_ROOT, 'archicad-addon', 'Sources')
COMMON_SCHEMA_FILE = os.path.join (SOURCES_DIR, 'RFIX', 'Images', 'CommonSchemaDefinitions.json')
DOCS_DIR = os.path.join (REPO_ROOT, 'docs', 'archicad-addon')
BUILD_ALL_SCRIPT = os.path.join (REPO_ROOT, 'archicad-addon', 'Tools', 'build_all_win.bat')


class GeneratorError (Exception):
    pass


def ReadText (path):
    # LF only, like the committed docs, even from a CRLF (Windows) checkout
    with open (path, encoding='utf-8', newline='') as f:
        return f.read ().replace ('\r\n', '\n')


def WriteText (path, content):
    with open (path, 'w', encoding='utf-8', newline='') as f:
        f.write (content)


def GetDocsArchicadVersion ():
    versions = [int (v) for v in re.findall (r'-DAC_VERSION=(\d+)', ReadText (BUILD_ALL_SCRIPT))]
    if not versions:
        raise GeneratorError ('No Archicad versions found in ' + BUILD_ALL_SCRIPT)
    return max (versions)


# ---------------------------------------------------------------------------
# Preprocessor: resolves the Archicad version conditions, keeps the rest
# ---------------------------------------------------------------------------

def EvaluateCondition (condition, acVersion):
    expr = condition.strip ()
    if re.fullmatch (r'\(?\s*\w+\s*\)?', expr):
        expr = 'defined (' + expr.strip ('() ') + ')'

    def Defined (match):
        name = match.group (1)
        m = re.fullmatch (r'ServerMainVers_(\d\d)(\d\d)', name)
        if m:
            return 'True' if int (m.group (1)) <= acVersion else 'False'
        raise GeneratorError ('Unknown macro in preprocessor condition: ' + condition)

    expr = re.sub (r'defined\s*\(?\s*(\w+)\s*\)?', Defined, expr)
    expr = re.sub (r'\bAC_VERSION\b', str (acVersion), expr)
    expr = expr.replace ('&&', ' and ').replace ('||', ' or ')
    expr = re.sub (r'!(?!=)', ' not ', expr)
    if not re.fullmatch (r'[\sTrueFalsnotdr()0-9<>=!]*', expr):
        raise GeneratorError ('Unsupported preprocessor condition: ' + condition)
    return bool (eval (expr))


def Preprocess (text, acVersion):
    # Every branch of a condition that is not about the Archicad version is
    # kept, but its directive lines stay in place so the interpreter refuses
    # to evaluate code inside it.
    output = []
    stack = []  # (resolved, keepingThisBranch, anyBranchTaken, parentActive)
    active = True
    for line in text.split ('\n'):
        m = re.match (r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)$', line)
        if not m:
            output.append (line if active else '')
            continue
        directive, rest = m.group (1), m.group (2).split ('//')[0].strip ()
        if directive in ('ifdef', 'ifndef', 'if'):
            condition = rest if directive == 'if' else ('defined (' + rest + ')')
            if directive == 'ifndef':
                condition = '!' + condition
            try:
                value = EvaluateCondition (condition, acVersion)
                stack.append ([True, value, value, active])
                active = active and value
                output.append ('')
            except GeneratorError:
                stack.append ([False, True, True, active])
                output.append (line if active else '')
        elif directive in ('elif', 'else'):
            if not stack:
                raise GeneratorError ('Unbalanced #' + directive)
            entry = stack[-1]
            if not entry[0]:
                output.append (line if entry[3] else '')
                continue
            if directive == 'else':
                value = not entry[2]
            else:
                value = not entry[2] and EvaluateCondition (rest, acVersion)
            entry[1] = value
            entry[2] = entry[2] or value
            active = entry[3] and value
            output.append ('')
        else:
            if not stack:
                raise GeneratorError ('Unbalanced #endif')
            entry = stack.pop ()
            active = entry[3]
            output.append ('' if entry[0] else (line if active else ''))
    return '\n'.join (output)


# ---------------------------------------------------------------------------
# Tokenizer
# ---------------------------------------------------------------------------

TOKEN_REGEX = re.compile (r'''
    (?P<ws>\s+|\\\n|//[^\n]*|/\*.*?\*/)
  | (?P<raw>(?:u8|u|U|L)?R"(?P<delim>[^(\s]{0,16})\((?P<rawbody>.*?)\)(?P=delim)")
  | (?P<str>(?:u8|u|U|L)?"(?:[^"\\\n]|\\.)*")
  | (?P<chr>'(?:[^'\\\n]|\\.)*')
  | (?P<pp>\#[^\n]*)
  | (?P<ident>[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*)
  | (?P<num>\d[\w.]*)
  | (?P<op>\+=|::|->|&&|\|\||==|!=|<=|>=|[-+*/%<>=!&|^~?:;,.(){}\[\]])
  | (?P<other>.)
''', re.S | re.X)

C_ESCAPES = {'n': '\n', 't': '\t', 'r': '\r', '0': '\0', '\\': '\\', '"': '"', "'": "'", '?': '?', 'a': '\a', 'b': '\b', 'f': '\f', 'v': '\v'}


def DecodeCString (literal):
    body = literal[literal.index ('"') + 1:-1]

    def Replace (match):
        esc = match.group (1)
        if esc[0] in C_ESCAPES and len (esc) == 1:
            return C_ESCAPES[esc]
        if esc[0] == 'x':
            return chr (int (esc[1:], 16))
        if esc[0] == 'u' or esc[0] == 'U':
            return chr (int (esc[1:], 16))
        raise GeneratorError ('Unsupported escape sequence: \\' + esc)

    return re.sub (r'\\(x[0-9A-Fa-f]+|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|.)', Replace, body)


def IterateTokens (text, pos=0):
    while pos < len (text):
        m = TOKEN_REGEX.match (text, pos)
        if not m:
            raise GeneratorError ('Cannot tokenize near: ' + text[pos:pos + 40])
        pos = m.end ()
        kind = m.lastgroup
        if kind == 'ws':
            continue
        if kind in ('raw', 'delim', 'rawbody'):
            yield ('str', m.group ('rawbody'))
        elif kind == 'str':
            yield ('str', DecodeCString (m.group ('str')))
        elif kind == 'ident':
            yield ('ident', re.sub (r'\s+', '', m.group ('ident')))
        else:
            yield (kind, m.group (kind))


def Tokenize (text):
    return list (IterateTokens (text))


# ---------------------------------------------------------------------------
# Source index
# ---------------------------------------------------------------------------

class Sources:
    def __init__ (self, acVersion):
        self.cpp = {}
        for path in sorted (glob.glob (os.path.join (SOURCES_DIR, '*.cpp'))):
            self.cpp[os.path.basename (path)] = Preprocess (ReadText (path), acVersion)
        self.hpp = '\n'.join (Preprocess (ReadText (p), acVersion) for p in sorted (glob.glob (os.path.join (SOURCES_DIR, '*.hpp'))))
        self.allCpp = '\n'.join (self.cpp.values ())

    def FindFunction (self, qualifiedName, file=None):
        # Returns (parameter names, body tokens) of a function definition.
        texts = [self.cpp[file]] if file else list (self.cpp.values ())
        pattern = re.compile (r'(?<![\w:])' + re.escape (qualifiedName).replace ('::', r'\s*::\s*') + r'\s*\(([^()]*)\)\s*(?:const\s*)?(?:override\s*)?\{')
        found = []
        for text in texts:
            for m in pattern.finditer (text):
                # skip calls: a definition is preceded by its return type
                before = text[max (0, m.start () - 200):m.start ()].rstrip ()
                if not re.search (r'[\w>*&]$', before) or re.search (r'\b(return|else|new)$', before):
                    continue
                found.append ((text, m))
        if not found:
            return None
        if len (found) > 1:
            raise GeneratorError ('Multiple definitions of ' + qualifiedName)
        text, m = found[0]
        params = []
        for param in m.group (1).split (','):
            param = param.split ('=')[0].strip ()
            if param and param != 'void':
                params.append (re.findall (r'\w+', param)[-1])
        tokens = []
        depth = 1
        for token in IterateTokens (text, m.end ()):
            if token == ('op', '{'):
                depth += 1
            elif token == ('op', '}'):
                depth -= 1
                if depth == 0:
                    return params, tokens
            tokens.append (token)
        raise GeneratorError ('Unterminated function ' + qualifiedName)

    def FindStringConstant (self, name):
        pattern = re.compile (r'(?:static\s+)?(?:constexpr\s+)?(?:const\s+)?(?:char\s*(?:const\s*)?\*\s*(?:const\s+)?|char\s+(?=\w+\s*\[)|GS::UniString\s+|GS::String\s+)' + name + r'\s*(?:\[\s*\])?\s*=\s*')
        matches = [(text, m) for text in self.cpp.values () for m in pattern.finditer (text)]
        if len (matches) != 1:
            return None
        text, m = matches[0]
        end = m.end ()
        tokens = []
        while not tokens or tokens[-1][0] == 'str':
            match = TOKEN_REGEX.match (text, end)
            if not match:
                return None
            end = match.end ()
            tokens += Tokenize (match.group (0))
        value = ''
        i = 0
        while tokens[i][0] == 'str':
            value += tokens[i][1]
            i += 1
        if i == 0 or tokens[i] != ('op', ';'):
            return None
        return value

    def GetBaseClass (self, className):
        m = re.search (r'class\s+' + className + r'\s*(?:final\s*)?:\s*public\s+(\w+)', self.hpp)
        return m.group (1) if m else None


# ---------------------------------------------------------------------------
# Interpreter
# ---------------------------------------------------------------------------

NO_VALUE = None


class Interpreter:
    def __init__ (self, sources):
        self.sources = sources
        self.cache = {}

    def CallMethod (self, className, methodName):
        # Virtual dispatch: the most derived definition wins; CommandBase
        # returns no schema.
        key = (className, methodName)
        if key not in self.cache:
            cls = className
            while cls and cls != 'CommandBase':
                function = self.sources.FindFunction (cls + '::' + methodName)
                if function is not None:
                    self.cache[key] = self.Run (cls + '::' + methodName, function, [], className)
                    break
                cls = self.sources.GetBaseClass (cls)
            else:
                if cls != 'CommandBase':
                    raise GeneratorError ('Cannot resolve ' + className + '::' + methodName)
                self.cache[key] = NO_VALUE
        return self.cache[key]

    def CallFunction (self, name, args, className):
        function = self.sources.FindFunction (name)
        if function is None:
            raise GeneratorError ('Unknown function ' + name)
        params, _ = function
        if len (params) != len (args):
            raise GeneratorError ('Wrong argument count calling ' + name)
        return self.Run (name, function, args, className)

    def Run (self, name, function, args, className):
        params, tokens = function
        state = _FunctionState (self, name, tokens, dict (zip (params, args)), className)
        try:
            result = state.RunBlock (top=True)
        except GeneratorError as e:
            raise GeneratorError (str (e) + '\n  in ' + name) from None
        return result


class _Return (Exception):
    def __init__ (self, value):
        self.value = value


class _FunctionState:
    def __init__ (self, interpreter, name, tokens, variables, className):
        self.interpreter = interpreter
        self.name = name
        self.tokens = tokens
        self.pos = 0
        self.variables = variables
        self.className = className

    def Peek (self, offset=0):
        index = self.pos + offset
        return self.tokens[index] if index < len (self.tokens) else ('eof', '')

    def Next (self):
        token = self.Peek ()
        self.pos += 1
        return token

    def Expect (self, value):
        token = self.Next ()
        if token[1] != value or token[0] in ('str',):
            raise GeneratorError ('Expected "' + value + '", found "' + str (token[1])[:40] + '"')

    def Fail (self, what):
        raise GeneratorError ('Unsupported ' + what + ' near "' + ' '.join (str (t[1])[:20] for t in self.tokens[self.pos:self.pos + 6]) + '"')

    def RunBlock (self, top=False):
        try:
            while self.Peek ()[0] != 'eof' and self.Peek () != ('op', '}'):
                self.RunStatement ()
        except _Return as r:
            if top:
                return r.value
            raise
        if top:
            raise GeneratorError ('Function does not return a value')

    def SkipBalanced (self, opening, closing):
        self.Expect (opening)
        depth = 1
        while depth:
            token = self.Next ()
            if token[0] == 'eof':
                raise GeneratorError ('Unbalanced "' + opening + '"')
            if token == ('op', opening):
                depth += 1
            elif token == ('op', closing):
                depth -= 1

    def SkipStatement (self):
        # Skips exactly one statement without running it; a nested if takes
        # its own else along, the caller's else is left to the caller.
        token = self.Peek ()
        if token == ('ident', 'if'):
            self.Next ()
            self.SkipBalanced ('(', ')')
            self.SkipStatement ()
            if self.Peek () == ('ident', 'else'):
                self.Next ()
                self.SkipStatement ()
            return
        if token == ('op', '{'):
            self.SkipBalanced ('{', '}')
            return
        while True:
            token = self.Next ()
            if token[0] == 'eof' or token == ('op', ';'):
                return
            if token == ('op', '('):
                self.pos -= 1
                self.SkipBalanced ('(', ')')
            elif token == ('op', '{'):
                self.pos -= 1
                self.SkipBalanced ('{', '}')

    def RunBody (self, execute):
        if not execute:
            self.SkipStatement ()
            return
        if self.Peek () == ('op', '{'):
            self.Next ()
            self.RunBlock ()
            self.Expect ('}')
        else:
            self.RunStatement ()

    def RunStatement (self):
        token = self.Peek ()
        if token[0] == 'pp':
            self.Fail ('preprocessor condition')
        if token == ('op', '{'):
            self.Next ()
            self.RunBlock ()
            self.Expect ('}')
            return
        if token == ('op', ';'):
            self.Next ()
            return
        if token == ('ident', 'return'):
            self.Next ()
            value = self.Expression ()
            self.Expect (';')
            raise _Return (value)
        if token == ('ident', 'if'):
            self.Next ()
            self.Expect ('(')
            condition = self.Condition ()
            self.Expect (')')
            self.RunBody (condition)
            if self.Peek () == ('ident', 'else'):
                self.Next ()
                self.RunBody (not condition)
            return
        # declaration: [static] [const] GS::UniString name [= expr | (expr)];
        index = 0
        while self.Peek (index)[1] in ('static', 'const'):
            index += 1
        if self.Peek (index)[1] in ('GS::UniString', 'GS::String') and self.Peek (index + 1)[0] == 'ident':
            self.pos += index + 1
            name = self.Next ()[1]
            if self.Peek () == ('op', '='):
                self.Next ()
                value = self.Expression ()
            elif self.Peek () == ('op', '('):
                self.Next ()
                value = self.Expression ()
                self.Expect (')')
            else:
                value = ''
            self.Expect (';')
            self.variables[name] = value
            return
        # assignment: name (+=|=) expr;
        if token[0] == 'ident' and token[1] in self.variables and self.Peek (1)[1] in ('=', '+='):
            self.Next ()
            op = self.Next ()[1]
            value = self.Expression ()
            self.Expect (';')
            if op == '=':
                self.variables[token[1]] = value
            else:
                if not isinstance (self.variables[token[1]], str) or not isinstance (value, str):
                    self.Fail ('string append')
                self.variables[token[1]] += value
            return
        self.Fail ('statement')

    def Condition (self):
        negate = False
        while self.Peek () == ('op', '!'):
            self.Next ()
            negate = not negate
        token = self.Next ()
        if token[0] == 'ident' and isinstance (self.variables.get (token[1]), bool):
            value = self.variables[token[1]]
        elif token[1] in ('true', 'false'):
            value = token[1] == 'true'
        else:
            self.pos -= 1
            self.Fail ('condition')
        return value != negate

    def Expression (self):
        value = self.Term ()
        while self.Peek () == ('op', '+'):
            self.Next ()
            right = self.Term ()
            if not isinstance (value, str) or not isinstance (right, str):
                self.Fail ('string concatenation')
            value += right
        return value

    def Arguments (self):
        self.Expect ('(')
        args = []
        while self.Peek () != ('op', ')'):
            args.append (self.Expression ())
            if self.Peek () == ('op', ','):
                self.Next ()
        self.Expect (')')
        return args

    def Term (self):
        token = self.Next ()
        kind, value = token
        if kind == 'str':
            while self.Peek ()[0] == 'str':
                value += self.Next ()[1]
            return value
        if token == ('op', '(') :
            result = self.Expression ()
            self.Expect (')')
            return result
        if token == ('op', '{') and self.Peek () == ('op', '}'):
            self.Next ()
            return NO_VALUE
        if kind != 'ident':
            self.pos -= 1
            self.Fail ('expression')
        if value in ('GS::NoValue', 'nullptr'):
            return NO_VALUE
        if value in ('true', 'false'):
            return value == 'true'
        if value in ('GS::UniString', 'GS::String') and self.Peek () == ('op', '('):
            args = self.Arguments ()
            if len (args) != 1:
                self.Fail ('string constructor')
            return args[0]
        if value in ('GS::UniString::Printf', 'GS::String::Printf'):
            args = self.Arguments ()
            return Printf (args[0], args[1:])
        if value in self.variables:
            return self.variables[value]
        if self.Peek () == ('op', '(') and self.Peek (1) == ('op', ')') and self.Peek (2) == ('op', '.'):
            # SomeCommand ().GetXxxSchema ()
            self.pos += 3
            method = self.Next ()
            self.Expect ('(')
            self.Expect (')')
            if method[0] != 'ident':
                self.Fail ('method call')
            return self.interpreter.CallMethod (value, method[1])
        if self.Peek () == ('op', '('):
            args = self.Arguments ()
            if value.startswith ('Get') and value.endswith ('Schema') and not args and '::' not in value:
                # calling an own virtual schema method
                return self.interpreter.CallMethod (self.className, value)
            return self.interpreter.CallFunction (value, args, self.className)
        constant = self.interpreter.sources.FindStringConstant (value)
        if constant is not None:
            return constant
        self.pos -= 1
        self.Fail ('identifier "' + value + '"')


def Printf (fmt, args):
    if not isinstance (fmt, str):
        raise GeneratorError ('Printf format is not a string')
    result = ''
    argIndex = 0
    pos = 0
    for m in re.finditer (r'%(%|[-+ #0]*\d*(?:\.\d+)?(?:ls|[a-zA-Z]))', fmt):
        result += fmt[pos:m.start ()]
        pos = m.end ()
        spec = m.group (1)
        if spec == '%':
            result += '%'
            continue
        if spec not in ('s', 'T', 'ls'):
            raise GeneratorError ('Unsupported Printf format %' + spec)
        if argIndex >= len (args) or not isinstance (args[argIndex], str):
            raise GeneratorError ('Printf argument mismatch')
        result += args[argIndex]
        argIndex += 1
    if argIndex != len (args):
        raise GeneratorError ('Printf argument mismatch')
    return result + fmt[pos:]


# ---------------------------------------------------------------------------
# Command registration
# ---------------------------------------------------------------------------

def GetCommandName (sources, interpreter, className):
    # Commands sharing a base class pass their name to its constructor, which
    # the base GetName returns; the others return it from their own GetName.
    m = re.search (re.escape (className) + r'\s*::\s*' + re.escape (className) + r'\s*\(\s*\)\s*:\s*(\w+)\s*\(\s*"([^"]+)"', sources.allCpp)
    if m and m.group (1) != 'CommandBase':
        return m.group (2)
    name = interpreter.CallMethod (className, 'GetName')
    if not isinstance (name, str):
        raise GeneratorError ('Cannot determine the command name of ' + className)
    return name


def CollectCommandGroups (sources, interpreter):
    function = sources.FindFunction ('Initialize', 'AddOnMain.cpp')
    if function is None:
        raise GeneratorError ('Initialize not found in AddOnMain.cpp')
    tokens = function[1]
    if any (t[0] == 'pp' for t in tokens):
        raise GeneratorError ('Unsupported preprocessor condition in Initialize')
    groups = {}
    order = []
    i = 0
    while i < len (tokens):
        t = tokens[i]
        if t == ('ident', 'CommandGroup') and tokens[i + 1][0] == 'ident' and tokens[i + 2] == ('op', '(') and tokens[i + 3][0] == 'str':
            groups[tokens[i + 1][1]] = (tokens[i + 3][1], [])
            i += 4
            continue
        if t[0] == 'ident' and t[1] == 'RegisterCommand' and tokens[i + 1] == ('op', '<'):
            className = tokens[i + 2][1]
            j = i + 3
            if tokens[j] != ('op', '>') or tokens[j + 1] != ('op', '('):
                raise GeneratorError ('Unexpected RegisterCommand call for ' + className)
            j += 2
            groupVar = tokens[j][1]
            if tokens[j + 1] != ('op', ',') or tokens[j + 2][0] != 'str' or tokens[j + 3] != ('op', ','):
                raise GeneratorError ('Unexpected RegisterCommand arguments for ' + className)
            version = tokens[j + 2][1]
            j += 4
            description = ''
            while tokens[j][0] == 'str':
                description += tokens[j][1]
                j += 1
            if tokens[j] != ('op', ')'):
                raise GeneratorError ('Unexpected RegisterCommand description for ' + className)
            if groupVar not in groups:
                raise GeneratorError ('Unknown command group ' + groupVar)
            groups[groupVar][1].append ({
                'name': GetCommandName (sources, interpreter, className),
                'version': version,
                'description': description,
                'inputScheme': interpreter.CallMethod (className, 'GetInputParametersSchema'),
                'outputScheme': interpreter.CallMethod (className, 'GetRawResponseSchema'),
            })
            i = j + 1
            continue
        if t == ('ident', 'AddCommandGroup'):
            order.append (groups[tokens[i + 2][1]])
            i += 3
            continue
        i += 1
    return order


# ---------------------------------------------------------------------------
# Output, formatted exactly like GenerateDocumentation in DeveloperTools.cpp
# ---------------------------------------------------------------------------

def FormatCommandDefinitions (groups):
    groupContents = []
    for groupName, commands in groups:
        commandContents = []
        for command in commands:
            commandContents.append ('''{
                "name": "%s",
                "version": "%s",
                "description": "%s",
                "inputScheme": %s,
                "outputScheme": %s
            }''' % (
                command['name'],
                command['version'],
                command['description'],
                'null' if command['inputScheme'] is None else command['inputScheme'],
                'null' if command['outputScheme'] is None else command['outputScheme']))
        groupContents.append ('''{
            "name": "%s",
            "commands": [%s]
        }''' % (groupName, ','.join (commandContents)))
    return 'var gCommands = [' + ','.join (groupContents) + '];'


def FormatCommonSchemaDefinitions ():
    return 'var gSchemaDefinitions = ' + ReadText (COMMON_SCHEMA_FILE) + ';'


def Validate (fileName, content, prefix):
    try:
        json.loads (content[len (prefix):-1])
    except ValueError as e:
        raise GeneratorError (fileName + ' would not be valid JSON (%s) - check the schemas and descriptions' % e)


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--check', action='store_true', help='exit with 1 when the docs are out of date, write nothing')
    args = parser.parse_args ()

    try:
        acVersion = GetDocsArchicadVersion ()
        sources = Sources (acVersion)
        interpreter = Interpreter (sources)
        outputs = {
            'command_definitions.js': FormatCommandDefinitions (CollectCommandGroups (sources, interpreter)),
            'common_schema_definitions.js': FormatCommonSchemaDefinitions (),
        }
        Validate ('command_definitions.js', outputs['command_definitions.js'], 'var gCommands = ')
        Validate ('common_schema_definitions.js', outputs['common_schema_definitions.js'], 'var gSchemaDefinitions = ')
    except GeneratorError as e:
        print ('error: ' + str (e), file=sys.stderr)
        return 2

    changed = []
    for fileName, content in outputs.items ():
        path = os.path.join (DOCS_DIR, fileName)
        if not os.path.exists (path) or ReadText (path) != content:
            changed.append (fileName)
            if not args.check:
                WriteText (path, content)

    if not changed:
        print ('docs/archicad-addon is up to date.')
        return 0
    if args.check:
        print ('docs/archicad-addon is out of date: ' + ', '.join (changed) + '. Run python tools/generate_addon_docs.py')
        return 1
    print ('Regenerated docs/archicad-addon: ' + ', '.join (changed))
    return 0


if __name__ == '__main__':
    sys.exit (main ())
