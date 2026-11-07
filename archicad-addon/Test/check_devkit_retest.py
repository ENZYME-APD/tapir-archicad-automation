"""Checks archicad-addon/devkit_retest.json against tools/api_coverage_rules.json.

Run by hand (no Archicad needed): python archicad-addon/Test/check_devkit_retest.py
Exits with 1 and lists the problems when the file is wrong; the format is in
archicad-addon/DEVKIT_RETEST.md.
"""
import json
import os
import sys

ADDON_DIR = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
RETEST_FILE = os.path.join (ADDON_DIR, 'devkit_retest.json')
RULES_FILE = os.path.join (os.path.dirname (ADDON_DIR), 'tools', 'api_coverage_rules.json')

REQUIRED_KEYS = ('problem', 'since', 'issue', 'howToRetest', 'unblocks', 'stillBroken')
OTHER_KINDS = ('duplicate', 'obsolete')


def SkippedIds (rules):
    skip = rules.get ('skip', {})
    ids = set ()
    for section in ('items', 'groups', 'patterns'):
        ids.update (skip.get (section, {}).keys ())
    return ids


def EntryItems (entry):
    if 'item' in entry and 'items' in entry:
        return None
    if 'item' in entry:
        return [entry['item']]
    return entry.get ('items')


def IsVersion (value):
    return isinstance (value, int) and not isinstance (value, bool) and value >= 25


def Check (retest, rules):
    errors = []
    skipped = SkippedIds (rules)
    seen = {}

    entries = retest.get ('entries')
    if not isinstance (entries, list):
        return ['"entries" must be a list']
    for index, entry in enumerate (entries):
        where = 'entries[{}]'.format (index)
        if not isinstance (entry, dict):
            errors.append (where + ' must be an object')
            continue
        items = EntryItems (entry)
        if not isinstance (items, list) or not items or not all (isinstance (i, str) and i for i in items):
            errors.append (where + ' needs either "item" (a string) or "items" (a list of strings), not both')
            items = []
        else:
            where += ' (' + items[0] + ')'
        for key in REQUIRED_KEYS:
            if key not in entry:
                errors.append (where + ' misses "' + key + '"')
        for key in ('problem', 'unblocks'):
            if key in entry and not (isinstance (entry[key], str) and entry[key].strip ()):
                errors.append (where + ' "' + key + '" must be a non-empty string')
        if 'since' in entry and not IsVersion (entry['since']):
            errors.append (where + ' "since" must be an Archicad version number such as 29')
        if 'issue' in entry and entry['issue'] is not None and not (isinstance (entry['issue'], int) and not isinstance (entry['issue'], bool) and entry['issue'] > 0):
            errors.append (where + ' "issue" must be an issue number or null')
        steps = entry.get ('howToRetest')
        if 'howToRetest' in entry and not (isinstance (steps, list) and steps and all (isinstance (s, str) and s.strip () for s in steps)):
            errors.append (where + ' "howToRetest" must be a non-empty list of steps (strings)')
        stillBroken = entry.get ('stillBroken')
        if 'stillBroken' in entry and not (isinstance (stillBroken, list) and all (IsVersion (v) for v in stillBroken)):
            errors.append (where + ' "stillBroken" must be a list of Archicad version numbers')
        for item in items:
            if item not in skipped:
                errors.append (where + ': "' + item + '" is not a skip in tools/api_coverage_rules.json')
            if item in seen:
                errors.append (where + ': "' + item + '" is also listed in ' + seen[item])
            seen[item] = where

    otherSkips = retest.get ('otherSkips')
    if not isinstance (otherSkips, dict):
        errors.append ('"otherSkips" must be an object')
        otherSkips = {}
    for item, kind in otherSkips.items ():
        where = 'otherSkips (' + item + ')'
        if kind not in OTHER_KINDS:
            errors.append (where + ': the kind must be one of ' + ', '.join (OTHER_KINDS))
        if item not in skipped:
            errors.append (where + ': "' + item + '" is not a skip in tools/api_coverage_rules.json')
        if item in seen:
            errors.append (where + ': "' + item + '" is also listed in ' + seen[item])
        seen[item] = where

    for item in sorted (skipped - set (seen)):
        errors.append ('the skip "' + item + '" is in neither "entries" (blocked by Archicad) nor "otherSkips" (duplicate or obsolete)')
    return errors


def Main ():
    with open (RETEST_FILE, encoding='utf-8') as f:
        retest = json.load (f)
    with open (RULES_FILE, encoding='utf-8') as f:
        rules = json.load (f)
    errors = Check (retest, rules)
    for error in errors:
        print ('devkit_retest.json: ' + error)
    if errors:
        return 1
    print ('devkit_retest.json is valid: {} entries, {} other skips.'.format (len (retest['entries']), len (retest['otherSkips'])))
    return 0


if __name__ == '__main__':
    sys.exit (Main ())
