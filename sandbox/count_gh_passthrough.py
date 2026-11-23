"""Counts the Grasshopper components that are pass-through (issue #3030).

A component is pass-through when its class only declares inputs and outputs,
builds the command's parameters from them one-to-one, calls one Tapir command and
returns its result, with no other logic. This decides whether the components can
be generated from the command schemas. Nothing is changed.

The rules are regular expressions in count_gh_passthrough_rules.txt, next to this
file: its first line is the prefix of every rule, each further line one rule,
which a file must match. A file is a component when it passes rules 1 and 2, and
pass-through when it passes them all. They are plain text rules over the whole
.cs file, so they are approximate: a word in a description string can still
match one (rules 9 and 10 only look at code lines, which start without a quote).

   1  declares a ComponentGuid (it is a component)
   2  is not an abstract class (a base class is not counted)
   3  holds one component class only
   4  sends a command itself (TryGetCadResponse, SetCadValues,
      SetCadValuesWithErrorMessages, TryGetConvertedCadValues, ElementMetadata,
      or a direct ToAddOn/ToArchicad/SendArchicadCommand call); without one,
      a base class sends it, or the component sends none
   5  sends no second command
   6  sends it to the Tapir Add-On (ToAddOn)
   7  sends no official Archicad command (ToArchicad)
   8  no loop, switch, else, LINQ call, ternary or ?? fallback
   9  every if on its own line is a guard (its block returns), such as reading an
      input or reporting an error
  10  no members besides the constructor, CommandName, AddInputs, AddOutputs,
      Solve, Icon, ComponentGuid and Exposure (no helpers, statics or other
      overrides)

The same rules run without Python, one pattern for all of them, with GNU grep:

  grep -rlzP "$(tr -d '\\n' < sandbox/count_gh_passthrough_rules.txt)" \\
      grasshopper-plugin/TapirGrasshopperPlugin/Components

Usage:
  python sandbox/count_gh_passthrough.py           totals per group, five examples each
  python sandbox/count_gh_passthrough.py --list    also every component with its verdict
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname (os.path.abspath (__file__))
ROOT = os.path.dirname (HERE)
COMPONENTS = os.path.join (ROOT, 'grasshopper-plugin', 'TapirGrasshopperPlugin', 'Components')
RULES_FILE = os.path.join (HERE, 'count_gh_passthrough_rules.txt')

RULE_NAMES = [
    'declares a ComponentGuid',
    'is not an abstract class',
    'several component classes in one file',
    'no command call in the file (a base class sends it, or none is sent)',
    'calls more than one command',
    'sends no command to the Tapir Add-On',
    'sends an official Archicad command',
    'loop, switch, else, LINQ, ternary or ?? fallback',
    'an if that is not a guard',
    'helper members, statics or other overrides',
]


def ReadRules ():
    with open (RULES_FILE, encoding='utf-8') as file:
        lines = [line.rstrip ('\n') for line in file if line.strip ()]
    prefix, rules = lines[0], lines[1:]
    if len (rules) != len (RULE_NAMES):
        sys.exit ('{} has {} rules, expected {}'.format (RULES_FILE, len (rules), len (RULE_NAMES)))
    # re.ASCII: \w, \s and \b as in grep -P without Unicode properties.
    combined = re.compile (prefix + ''.join (rules), re.ASCII)
    single = [re.compile (prefix + rule, re.ASCII) for rule in rules]
    return combined, single


def ComponentFiles ():
    for folder, _, names in sorted (os.walk (COMPONENTS)):
        for name in sorted (names):
            if name.endswith ('.cs'):
                yield os.path.join (folder, name)


def GroupOf (path):
    parts = os.path.relpath (path, COMPONENTS).split (os.sep)
    return parts[0] if len (parts) > 1 else '(top level)'


def Main ():
    parser = argparse.ArgumentParser (description='Counts the pass-through Grasshopper components.')
    parser.add_argument ('--list', action='store_true', help='list every component with its verdict')
    args = parser.parse_args ()

    combined, single = ReadRules ()
    groups = {}
    examples = {True: [], False: []}
    reasons = {}
    verdicts = []
    for path in ComponentFiles ():
        # newline='' keeps the file as it is on disk, as grep reads it.
        with open (path, encoding='utf-8', errors='replace', newline='') as file:
            text = file.read ()
        if not (single[0].search (text) and single[1].search (text)):
            continue
        passThrough = combined.search (text) is not None
        reason = None
        if not passThrough:
            reason = next (index for index in range (2, len (single)) if not single[index].search (text))
            reasons[reason] = reasons.get (reason, 0) + 1
        group = GroupOf (path)
        counts = groups.setdefault (group, [0, 0])
        counts[0 if passThrough else 1] += 1
        relative = os.path.relpath (path, ROOT).replace (os.sep, '/')
        examples[passThrough].append ((relative, reason))
        verdicts.append ((relative, passThrough, reason))

    total = [sum (c[0] for c in groups.values ()), sum (c[1] for c in groups.values ())]
    print ('| Group | Components | Pass-through | Not pass-through |')
    print ('|---|---:|---:|---:|')
    for group in sorted (groups):
        passCount, otherCount = groups[group]
        print ('| {} | {} | {} | {} |'.format (group, passCount + otherCount, passCount, otherCount))
    print ('| **Total** | **{}** | **{}** | **{}** |'.format (sum (total), total[0], total[1]))
    print ()
    print ('Not pass-through, by the first rule a component fails:')
    print ()
    for index in sorted (reasons):
        print ('- rule {}, {}: {}'.format (index + 1, RULE_NAMES[index], reasons[index]))
    print ()
    print ('Examples of pass-through components:')
    for relative, _ in examples[True][:5]:
        print ('- ' + relative)
    print ()
    print ('Examples of components that are not pass-through:')
    for relative, reason in examples[False][:5]:
        print ('- {} (rule {}: {})'.format (relative, reason + 1, RULE_NAMES[reason]))
    if args.list:
        print ()
        for relative, passThrough, reason in verdicts:
            print ('{}\t{}'.format (relative, 'pass-through' if passThrough else
                                    'rule {}: {}'.format (reason + 1, RULE_NAMES[reason])))


if __name__ == '__main__':
    Main ()
