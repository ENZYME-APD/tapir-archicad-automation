"""Checks archicad-addon/Examples: one simple example script per Tapir command.

The command reference (docs/archicad-addon) shows each command's example in a popdown, fetched
from GitHub as archicad-addon/Examples/<GroupFolder>/<CommandName>.py. So:

  * every command registered in AddOnMain.cpp has Examples/<GroupFolder>/<CommandName>.py,
    where <GroupFolder> comes from the name of its command group (ExampleFolder below; the
    same rule is in docs/archicad-addon/renderer.js); the Developer Commands have none,
  * each of them starts with a module docstring (the docs show it as the example's
    description) and calls its own command with aclib.RunTapirCommand ('<CommandName>', ...),
  * no example passes debug to aclib (they print what they show, e.g. with aclib.PrintJson),
  * each group folder has the aclib.py shim, so `python Examples/<GroupFolder>/<Name>.py` finds
    the aclib/ helper package,
  * there are no other .py files under Examples/ besides aclib/ and the shims: tests go to
    archicad-addon/Test/Scripts/, multi-command workflows and tools to archicad-addon/Workflows/.

The registered commands are read from the sources the way tools/generate_addon_docs.py reads
them (no Archicad needed). Standard library only.

While the one-per-command examples are being written this fails with the commands that have no
example yet; that is expected until all of them are merged.

Usage: python tools/check_examples.py [--report]
  --report  also print the counts (commands, examples, missing, problems)
Exits with 1 and the list of problems when the check fails, 2 when the sources cannot be read.
"""

import argparse
import ast
import os
import re
import sys

sys.path.insert (0, os.path.dirname (os.path.abspath (__file__)))
import generate_addon_docs  # noqa: E402

EXAMPLES_DIR = os.path.join (generate_addon_docs.REPO_ROOT, 'archicad-addon', 'Examples')
HELPER_DIR = 'aclib'
SHIM_FILE = 'aclib.py'
GROUPS_WITHOUT_EXAMPLES = {'Developer Commands'}


def ExampleFolder (groupName):
    """The Examples subfolder of a command group: "Custom Data Commands" -> "CustomData",
    "MEP Commands (extended)" -> "MEP", "Script UI Commands" -> "ScriptUI". The parenthesised
    part and a trailing "Commands" are dropped, the words are joined in PascalCase and
    all-caps words stay as they are. ExampleFolder in docs/archicad-addon/renderer.js is the same."""
    name = re.sub (r'\s*\([^)]*\)', '', groupName).strip ()
    name = re.sub (r'\s+Commands$', '', name)
    return ''.join (word if word.isupper () else word[:1].upper () + word[1:] for word in name.split ())


class _NamesOnly:
    """Answers GetName like the docs generator's interpreter, but skips evaluating the schemas,
    which this check does not need."""

    def __init__ (self, interpreter):
        self.interpreter = interpreter

    def CallMethod (self, className, methodName, *args):
        if methodName in ('GetInputParametersSchema', 'GetRawResponseSchema'):
            return None
        return self.interpreter.CallMethod (className, methodName, *args)


def GetRegisteredCommands ():
    """[(group name, [command name, ...]), ...] in the order of AddOnMain.cpp."""
    acVersion = generate_addon_docs.GetDocsArchicadVersion ()
    sources = generate_addon_docs.Sources (acVersion)
    interpreter = _NamesOnly (generate_addon_docs.Interpreter (sources))
    # The generator finds a command's name by searching the sources once per command, which takes
    # a minute for all of them. The two usual forms are found here in one pass each: a constructor
    # passing the name to a base class other than CommandBase (what the generator checks first),
    # and a GetName of the class itself that returns a string literal (what the interpreter's
    # virtual call finds first). Anything else still goes to the interpreter.
    constructorNames = {}
    for m in re.finditer (r'\b(\w+)\s*::\s*(\w+)\s*\(\s*\)\s*:\s*(\w+)\s*\(\s*"([^"]+)"', sources.allCpp):
        if m.group (1) == m.group (2):
            constructorNames.setdefault (m.group (1), (m.group (3), m.group (4)))
    literalNames = {}
    for m in re.finditer (r'\b(\w+)\s*::\s*GetName\s*\(\s*\)\s*const\s*\{\s*return\s*"([^"]+)"\s*;\s*\}', sources.allCpp):
        literalNames.setdefault (m.group (1), m.group (2))
    originalGetCommandName = generate_addon_docs.GetCommandName

    def GetCommandName (sources, interpreter, className):
        found = constructorNames.get (className)
        if found and found[0] != 'CommandBase':
            return found[1]
        if className in literalNames:
            return literalNames[className]
        name = interpreter.CallMethod (className, 'GetName')
        if not isinstance (name, str):
            raise generate_addon_docs.GeneratorError ('Cannot determine the command name of ' + className)
        return name

    generate_addon_docs.GetCommandName = GetCommandName
    try:
        groups = generate_addon_docs.CollectCommandGroups (sources, interpreter)
    finally:
        generate_addon_docs.GetCommandName = originalGetCommandName
    return [(groupName, [command['name'] for command in commands]) for groupName, commands in groups]


def CalledTapirCommands (tree):
    """The names given as string literals to aclib.RunTapirCommand / RunTapirCommand."""
    names = set ()
    for node in ast.walk (tree):
        if not isinstance (node, ast.Call):
            continue
        func = node.func
        funcName = func.attr if isinstance (func, ast.Attribute) else func.id if isinstance (func, ast.Name) else None
        if funcName != 'RunTapirCommand':
            continue
        candidates = node.args[:1] + [keyword.value for keyword in node.keywords if keyword.arg == 'command']
        for arg in candidates:
            if isinstance (arg, ast.Constant) and isinstance (arg.value, str):
                names.add (arg.value)
    return names


def CheckExample (path, commandName):
    """The problems of one example script, as a list of strings."""
    try:
        with open (path, encoding='utf-8') as f:
            tree = ast.parse (f.read (), filename=path)
    except (SyntaxError, UnicodeDecodeError) as e:
        return ['does not parse: {}'.format (e)]
    problems = []
    docstring = ast.get_docstring (tree)
    if not docstring or not docstring.strip ():
        problems.append ('does not start with a module docstring')
    if commandName not in CalledTapirCommands (tree):
        problems.append ("does not call aclib.RunTapirCommand ('{}', ...)".format (commandName))
    for node in ast.walk (tree):
        if (isinstance (node, ast.Call) and isinstance (node.func, ast.Attribute) and node.func.attr in ('RunTapirCommand', 'RunCommand')
                and any (keyword.arg == 'debug' for keyword in node.keywords)):
            problems.append ('passes debug to aclib.{} (line {}); print what the example shows with aclib.PrintJson instead'.format (node.func.attr, node.lineno))
    return problems


def ListPythonFiles ():
    """Every .py file under Examples/ outside aclib/, as a path relative to Examples/."""
    result = []
    for root, dirs, files in os.walk (EXAMPLES_DIR):
        relativeRoot = os.path.relpath (root, EXAMPLES_DIR)
        dirs[:] = [d for d in dirs if d != '__pycache__' and not (relativeRoot == '.' and d == HELPER_DIR)]
        result += [os.path.normpath (os.path.join (relativeRoot, f)).replace (os.sep, '/') for f in files if f.endswith ('.py')]
    return sorted (result)


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--report', action='store_true', help='also print the counts')
    args = parser.parse_args ()

    try:
        groups = GetRegisteredCommands ()
    except generate_addon_docs.GeneratorError as e:
        print ('error: ' + str (e), file=sys.stderr)
        return 2

    expected = {}  # relative path -> command name
    folders = set ()
    for groupName, commands in groups:
        if groupName in GROUPS_WITHOUT_EXAMPLES:
            continue
        folder = ExampleFolder (groupName)
        folders.add (folder)
        for name in commands:
            expected[folder + '/' + name + '.py'] = name
    allowed = set (expected) | set (folder + '/' + SHIM_FILE for folder in folders)

    present = ListPythonFiles ()
    missing = [path for path in expected if path not in present]
    unknown = [path for path in present if path not in allowed]
    missingShims = sorted (folder + '/' + SHIM_FILE for folder in folders if folder + '/' + SHIM_FILE not in present)
    problems = []
    checked = 0
    for path, name in expected.items ():
        fullPath = os.path.join (EXAMPLES_DIR, path)
        if os.path.isfile (fullPath):
            checked += 1
            problems += ['{} {}'.format (path, problem) for problem in CheckExample (fullPath, name)]

    if args.report:
        print ('Commands that need an example: {} in {} folders'.format (len (expected), len (folders)))
        print ('With an example:               {}'.format (checked))
        print ('Without an example:            {}'.format (len (missing)))
        print ('Examples with problems:        {}'.format (len (set (p.split (' ', 1)[0] for p in problems))))
        print ('Not a command example:         {}'.format (len (unknown)))
        print ('Missing aclib.py shims:        {}'.format (len (missingShims)))

    if not missing and not unknown and not problems and not missingShims:
        print ('archicad-addon/Examples is complete: one example for each of the {} commands.'.format (len (expected)))
        return 0
    if missing:
        print ('Commands without archicad-addon/Examples/<GroupFolder>/<CommandName>.py ({}):'.format (len (missing)))
        print ('\n'.join ('  ' + path for path in missing))
    if problems:
        print ('Examples with problems ({}):'.format (len (problems)))
        print ('\n'.join ('  ' + problem for problem in problems))
    if missingShims:
        print ('Group folders without the aclib.py shim ({}):'.format (len (missingShims)))
        print ('\n'.join ('  ' + path for path in missingShims))
    if unknown:
        print ('Files in archicad-addon/Examples that are not the example of a registered command ({}); '
               'tests go to archicad-addon/Test/Scripts/, workflows to archicad-addon/Workflows/:'.format (len (unknown)))
        print ('\n'.join ('  ' + path for path in unknown))
    return 1


if __name__ == '__main__':
    sys.exit (main ())
