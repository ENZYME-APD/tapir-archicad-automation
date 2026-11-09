"""Writes the short summary that the weekly release puts in its release notes.

    python tools/release_notes.py --previous 1.8.0 --current HEAD --version 2.0.0 \
        --generated generated.md > summary.md

GitHub's generated notes list every merged pull request, which runs to hundreds of lines for
a busy week. This writes a summary instead, from what changed between the two refs:
  - the new JSON commands, counted per command group, from docs/archicad-addon/command_definitions.js
    (the release regenerates it before tagging, so it is exact at every tag);
  - the new Grasshopper components (files named *Component.cs under Components/);
  - the new marketplace scripts (marketplace/<slug>/listing.json), by name;
  - the merged pull requests and the people who made them, and the "New Contributors"
    section, read from GitHub's generated notes (--generated).
The full list stays one click away: the summary ends with the compare link. weekly_release.yml
adds the installer links above it and the code signing line below it. Standard library only.
"""

import argparse
import collections
import json
import re
import subprocess
import sys

REPO_URL = 'https://github.com/ENZYME-APD/tapir-archicad-automation'
SITE_URL = 'https://archicad-api.com'
COMMAND_DEFINITIONS = 'docs/archicad-addon/command_definitions.js'
COMPONENTS_FOLDER = 'grasshopper-plugin/TapirGrasshopperPlugin/Components'
BOT_LOGIN = re.compile (r'(\[bot\]$|^claude$|^github-actions$|^dependabot)', re.I)
PULL_LINE = re.compile (r'^\* (?P<title>.+) by @(?P<login>\S+) in (?P<url>\S+)\s*$')


def Git (*args):
    return subprocess.run (['git'] + list (args), capture_output=True, text=True, check=True).stdout


def FileAt (ref, path):
    try:
        return Git ('show', ref + ':' + path)
    except subprocess.CalledProcessError:
        return None


def CommandsAt (ref):
    """{command name: group name} of the documented commands at ref."""
    text = FileAt (ref, COMMAND_DEFINITIONS)
    if text is None:
        return {}
    text = text.strip ()
    groups = json.loads (text[text.index ('=') + 1:].rstrip (';'))
    commands = {}
    for group in groups:
        name = re.sub (r'\s*Commands\b', '', group['name']).strip () or group['name']
        name = re.sub (r'\s*\(extended\)$', '', name)
        for command in group['commands']:
            commands[command['name']] = name
    return commands


def ComponentsAt (ref):
    try:
        files = Git ('ls-tree', '-r', '--name-only', ref, COMPONENTS_FOLDER).split ()
    except subprocess.CalledProcessError:
        return set ()
    return {f for f in files if f.endswith ('Component.cs')}


def MarketplaceAt (ref):
    """{slug: listing name} of the marketplace scripts at ref."""
    try:
        files = Git ('ls-tree', '-r', '--name-only', ref, 'marketplace').split ()
    except subprocess.CalledProcessError:
        return {}
    scripts = {}
    for path in files:
        parts = path.split ('/')
        if len (parts) == 3 and parts[2] == 'listing.json':
            try:
                listing = json.loads (FileAt (ref, path) or '{}')
            except ValueError:
                listing = {}
            scripts[parts[1]] = listing.get ('name') or parts[1]
    return scripts


def ReadGenerated (path):
    """(pull requests as (title, login), the New Contributors section or '') of GitHub's notes."""
    if not path:
        return [], ''
    with open (path, encoding='utf-8') as file:
        text = file.read ()
    pulls = [(m.group ('title'), m.group ('login')) for m in map (PULL_LINE.match, text.splitlines ()) if m]
    newContributors = ''
    match = re.search (r'^## New Contributors\s*\n(.*?)(?=^## |^\*\*Full Changelog|\Z)', text, re.M | re.S)
    if match:
        newContributors = match.group (1).strip ()
    return pulls, newContributors


def Plural (count, word, plural=None):
    return '{} {}'.format (count, word if count == 1 else (plural or word + 's'))


def Summary (previous, current, version, generatedPath):
    for ref in (previous, current):
        # A ref that is not there would count everything as new: fail, and the
        # workflow falls back to GitHub's list.
        if subprocess.run (['git', 'rev-parse', '--verify', '--quiet', ref + '^{commit}'], capture_output=True).returncode != 0:
            raise SystemExit ('release_notes.py: unknown ref ' + ref)
    lines = ['## What changed since {}'.format (previous), '']

    before, after = CommandsAt (previous), CommandsAt (current)
    added = sorted (set (after) - set (before))
    if added:
        perGroup = collections.Counter (after[name] for name in added)
        groups = ', '.join ('{} {}'.format (group, count) for group, count in perGroup.most_common ())
        lines.append ('- **{}** ({} → {}): {}. See the [command reference]({}/archicad-addon/).'.format (
            Plural (len (added), 'new JSON command'), len (before), len (after), groups, SITE_URL))
        if len (added) <= 12:
            lines.append ('  ' + ', '.join ('`{}`'.format (name) for name in added))

    componentsBefore, componentsAfter = ComponentsAt (previous), ComponentsAt (current)
    newComponents = len (componentsAfter - componentsBefore)
    if newComponents:
        lines.append ('- **{}** in the Grasshopper plugin ({} → {}).'.format (
            Plural (newComponents, 'new component'), len (componentsBefore), len (componentsAfter)))

    scriptsBefore, scriptsAfter = MarketplaceAt (previous), MarketplaceAt (current)
    newScripts = [scriptsAfter[slug] for slug in sorted (set (scriptsAfter) - set (scriptsBefore))]
    if newScripts:
        names = ': ' + ', '.join (newScripts) if len (newScripts) <= 8 else ''
        lines.append ('- **{}** in the [script marketplace]({}/scripts/){}.'.format (
            Plural (len (newScripts), 'new script'), SITE_URL, names))

    pulls, newContributors = ReadGenerated (generatedPath)
    if pulls:
        people = collections.Counter (login for _, login in pulls if not BOT_LOGIN.search (login))
        botPulls = sum (1 for _, login in pulls if BOT_LOGIN.search (login))
        merged = '- **{}** merged'.format (Plural (len (pulls), 'pull request'))
        if people:
            merged += ', by ' + ', '.join ('@{} ({})'.format (login, count) for login, count in people.most_common ())
        if botPulls:
            merged += ('' if not people else ' and') + ' the Tapir issue bot ({})'.format (botPulls)
        lines.append (merged + '.')

    if len (lines) == 2:
        lines.append ('- Fixes and smaller changes.')

    if newContributors:
        lines += ['', '**New contributors:** welcome, and thank you!', '', newContributors]

    lines += ['', '**Full Changelog** (every pull request): {}/compare/{}...{}'.format (REPO_URL, previous, version)]
    return '\n'.join (lines) + '\n'


def main ():
    parser = argparse.ArgumentParser (description=__doc__.split ('\n')[0])
    parser.add_argument ('--previous', required=True, help='the previous release tag')
    parser.add_argument ('--current', default='HEAD', help='the ref being released (default: HEAD)')
    parser.add_argument ('--version', required=True, help='the version being released, for the compare link')
    parser.add_argument ('--generated', help="GitHub's generated release notes (releases/generate-notes), as a file")
    args = parser.parse_args ()
    sys.stdout.write (Summary (args.previous, args.current, args.version, args.generated))


if __name__ == '__main__':
    main ()
